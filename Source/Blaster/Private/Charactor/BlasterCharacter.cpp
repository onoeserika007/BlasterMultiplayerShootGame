// Fill out your copyright notice in the Description page of Project Settings.


#include "Charactor/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Public/Charactor/BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Blaster/Public/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"
#include "Blaster/Public/BlasterTypes/WeaponTypes.h"

ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// this may cause bug when using seamless travel
	// AutoPossessPlayer = EAutoReceiveInput::Player0;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	//Combat->SetupAttachment(RootComponent);
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate.Yaw = 850.f;

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	DissolveTimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
	if (DissolveTimelineComponent) {
		UE_LOG(LogTemp, Warning, TEXT("DissolveTimeline Constructed in constructor."));
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABlasterCharacter, OverlappingWeapon);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat) {
		Combat->Character = this;
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f;
}

void ABlasterCharacter::PlayFireMontage(bool bIsAiming)
{
	if (Combat && Combat->EquippedWeapon) {
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && FireWeaponMontage) {
			AnimInstance->Montage_Play(FireWeaponMontage);
			FName SectionName = bIsAiming ? FName("RifleAim") : FName("RifleHip");
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat && Combat->EquippedWeapon) {
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && HitReactMontage) {
			AnimInstance->Montage_Play(FireWeaponMontage);
			FName SectionName = FName("FromFront");
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage) {
		AnimInstance->Montage_Play(ElimMontage);
		FName SectionName = FName("Default");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage) {
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName = FName("Rifle");
		switch (Combat->EquippedWeapon->GetWeaponType()) {
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::Elim()
{
	if (HasAuthority()) {
		if (Combat && Combat->EquippedWeapon) {
			Combat->EquippedWeapon->Dropped();
		}
		MulticastElim();
		GetWorldTimerManager().SetTimer(
			ElimTimer,
			this,
			&ThisClass::ElimTimerFinished,
			RespawnDelay
		);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	// however this is only valid on server, we need do this in destroyed in BlasterCharacter.
	//if (ElimBotComponent) {
	//	ElimBotComponent->DestroyComponent();
	//}

	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode) {
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (BlasterPC) {
		BlasterPC->SetHUDWeaponAmmo(0);
	}
	PlayElimMontage();
	bElimmed = true;

	// start dissolve effect
	if (DissolveMaterialInstance) {
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.6f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 35.0f);
	}
	StartDissolve();

	// Disable character movemnet
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (BlasterPC) {
		DisableInput(BlasterPC);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn elim Bot
	if (ElimBotEffect) {
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.0f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}

	if (ElimBotSound) {
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	UpdateHUDHealth();
	//PlayHitReactMontage();
	if (Health == 0.0f) {
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode) {
			BlasterPC = BlasterPC == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPC;
			ABlasterPlayerController* AttackerPC = Cast<ABlasterPlayerController>(InstigatedBy);
			BlasterGameMode->PlayerElimilated(this, BlasterPC, AttackerPC);
		}
	}
}

void ABlasterCharacter::Destroyed()
{
	if (ElimBotComponent) {
		ElimBotComponent->DestroyComponent();
	}
	Super::Destroyed();
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// client case && server case
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled()) {
		AimOffset(DeltaTime);
	}
	// sim will be handle on movement replicated
	else {
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f) {
			OnRep_ReplicatedMovement();
		}
	}
	HideCharacterIfCameraClose();

	// poll init | HUD score
	PollInit();
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* pc = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				InputSystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		if (IA_Move) {
			EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ThisClass::Move);
		}

		if (IA_Look) {
			EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ThisClass::Look);
		}

		if (IA_Jump) {
			EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ThisClass::Jump);
		}

		if (IA_Equip) {
			EnhancedInputComponent->BindAction(IA_Equip, ETriggerEvent::Started, this, &ThisClass::Equip);
		}

		if (IA_Crouch) {
			EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &ThisClass::CrouchCompleted);
			EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Canceled, this, &ThisClass::CrouchCanceled);
			EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Ongoing, this, &ThisClass::CrouchOngoing);
		}

		if (IA_Aim) {
			EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Started, this, &ThisClass::AimTriggered);
			EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Canceled, this, &ThisClass::AimTriggered);
		}

		if (IA_Fire) {
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Started, this, &ThisClass::Fire);
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Completed, this, &ThisClass::FireEnd);
		}

		if (IA_Reload) {
			EnhancedInputComponent->BindAction(IA_Reload, ETriggerEvent::Started, this, &ThisClass::Reload);
		}
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// this is needed since when OnPossess in Controller is called the HUD maybe not ready.
	UpdateHUDHealth();

	if (HasAuthority()) {
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}

	ENetRole LocalRole = GetLocalRole();

	auto setRole = [=]()-> auto {
		switch (LocalRole) {
		case ENetRole::ROLE_Authority:
			return FString("Authority");
		case ENetRole::ROLE_AutonomousProxy:
			return FString("Autonomous Proxy");
		case ENetRole::ROLE_SimulatedProxy:
			return FString("Simulated Proxy");
		case ENetRole::ROLE_None:
			return FString("None");
		}
		return FString("Not Set");
		};
	FString RoleString = setRole();

	if (!DissolveTimelineComponent) {
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed to construct DissolveTimeline component. Trying to reconstruct in BeginPlay."), *RoleString);
		//if (!DissolveTimelineComponent) {
		//	UE_LOG(LogTemp, Warning, TEXT("%s: Reconstruct Failed."), *RoleString);
		//}
		//else {
		//	UE_LOG(LogTemp, Warning, TEXT("%s: Reconstruct Success."), *RoleString);
		//}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("%s: DissolveTimeline component constructed."), *RoleString);
	}
	
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPC = BlasterPC == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPC;
	if (BlasterPC) {
		BlasterPC->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr) {
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState) {
			BlasterPlayerState->AddToScore(0.0f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

void ABlasterCharacter::Move(const FInputActionInstance& Instance)
{
	const auto& values = Instance.GetValue().Get<FVector2D>();

	FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	FVector rightVec = UKismetMathLibrary::GetRightVector(YawRotation);
	FVector forwardVec = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(rightVec, values.X);
	AddMovementInput(forwardVec, values.Y);

//	if (GEngine) {
//	GEngine->AddOnScreenDebugMessage(
//		-1,
//		2,
//		FColor::Cyan,
//		FString(TEXT("Move triggered."))
//	);
//}
}

void ABlasterCharacter::Look(const FInputActionInstance& Instance)
{
	const auto& values = Instance.GetValue().Get<FVector2D>();

	AddControllerPitchInput(-values.Y);
	AddControllerYawInput(values.X);
}

void ABlasterCharacter::Jump()
{
	if (bIsPersitentCrouching) {

	}
	Super::Jump();
	UnCrouch();
}

void ABlasterCharacter::Equip()
{
	if (Combat) {
		//if (HasAuthority()) {
		//	Combat->EquipWeapon(OverlappingWeapon);
		//}
		//else {
		//	ServerEuipped();
		//}

		// do this locally is uneccessary, server will handle this, but do it locally for low delay
		//Combat->EquipWeapon(OverlappingWeapon);
		ServerEuipped();
		
	}
}

void ABlasterCharacter::ServerEuipped_Implementation()
{
	if (Combat) {
		//if (GEngine) {
		//	GEngine->AddOnScreenDebugMessage(
		//		-1,
		//		2,
		//		FColor::Cyan,
		//		FString(TEXT("Euip triggered."))
		//	);
		//}
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::CrouchCompleted()
{
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			2,
			FColor::Cyan,
			FString(TEXT("Crouch"))
		);
	}
	if (bIsCrouched) {
		Super::UnCrouch();
	}
	else {
		Super::Crouch();
	}
}

void ABlasterCharacter::CrouchOngoing()
{
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			2,
			FColor::Cyan,
			FString(TEXT("Crouching"))
		);
	}
	bIsPersitentCrouching = true;
	Super::Crouch();
}

void ABlasterCharacter::CrouchCanceled()
{
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			2,
			FColor::Cyan,
			FString(TEXT("UnCrouch"))
		);
	}
	bIsPersitentCrouching = false;
	Super::UnCrouch();
}

void ABlasterCharacter::AimTriggered()
{
	if (Combat) {
		Combat->SetAiming(!Combat->bIsAming);
	}
}

void ABlasterCharacter::Fire()
{
	if (Combat) {
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireEnd()
{
	if (Combat) {
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon) {
		float Speed = GetVelocity().Size2D();
		bool bIsInAir = GetCharacterMovement()->IsFalling();

		// on the ground and still and not aiming
		if (Speed == 0.f && !bIsInAir && !IsAiming()) {
			bRotateRootBone = true;
			FRotator CurrentAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
			FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
			AO_Yaw = DeltaAimRotation.Yaw;
			if (TurningInPlace == ETurningInPlace::ETIP_NotTurning) {
				InterpAO_Yaw = AO_Yaw;
			}
			bUseControllerRotationYaw = false;
			bUseControllerRotationYaw = true;
			TurnInPlace(DeltaTime);
		}

		// moving or in air or aiming
		if (Speed > 0.f || bIsInAir || IsAiming()) {
			bRotateRootBone = false;
			StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
			AO_Yaw = 0.0f;
			bUseControllerRotationYaw = true;
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		CalculateAO_Pitch();
		//if (HasAuthority() && !IsLocallyControlled()) {
		//	UE_LOG(LogTemp, Warning, TEXT("AO_pitch: %f"), AO_Pitch);

		//}
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	// ue network will pack angle to 0~360
	// correction on other machine
	if (AO_Pitch > 180.f && !IsLocallyControlled()) {
		FVector2D InRange(270.0f, 360.0f);
		FVector2D OutRange(-90.f, 0.0f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat && Combat->EquippedWeapon) {
		bRotateRootBone = false;
		float Speed = GetVelocity().Size2D();
		if (Speed > 0.0f) {
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			return;
		}

		CalculateAO_Pitch();
		ProxyRotationLastFrame = ProxyRotation;
		ProxyRotation = GetActorRotation();
		ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

		if (FMath::Abs(ProxyYaw) > TurnThreshold) {
			if (ProxyYaw > TurnThreshold) {
				TurningInPlace = ETurningInPlace::ETIP_Right;
			}
			else {
				TurningInPlace = ETurningInPlace::ETIP_Left;
			}
		}
		else {
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
	}
}

void ABlasterCharacter::Reload()
{
	if (Combat) {
		Combat->Reload();
	}
}


// rep notification only call on client 
// and is call only when replicated, and the OverlappingWeapon has been updated
// the Weapon on client is copy, so we can have different visibility from the server.
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (LastWeapon) {
		LastWeapon->ShowPickupWidget(false);
	}

	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(true);
	}
	else {
		// but ... no ptr, how can you did this?
		// OverlappingWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	//UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
	if (AO_Yaw > 90.f) {
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f) {
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	//else {
	//	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	//}

	// the GetBaseAimRotation() is oriented with the controller, this is just adjusting the anim
	// an interp from InterpAO_Yaw is triggered when turning is triggered, the interp for yaw to 0 is just decreasing the degrees that the root bone turn back.
	// and the initial value for Interp_Yaw is stored before turning, just prepared for this.
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning) {
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.0f, DeltaTime, 4.0f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f) {
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		}
	}
}

void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (IsLocallyControlled()) {
		if (FollowCamera) {
			if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold) {
				GetMesh()->SetVisibility(false);
				if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
					Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
				}
			}
			else {
				GetMesh()->SetVisibility(true);
				if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
					Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
				}
			}
		}
	}
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			1,
			1,
			FColor::Yellow,
			FString("CurvePlaying")
		);
	}
	if (DynamicDissolveMaterialInstance) {
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimelineComponent) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(
				-1,
				1,
				FColor::Yellow,
				FString("Start play curve.")
			);
		}
		DissolveTimelineComponent->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimelineComponent->Play();
	}
	if (!DissolveTimelineComponent) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(
				-1,
				1,
				FColor::Yellow,
				FString("Timeline is nullptr.")
			);
		}
	}
}

// called on the server
// handling the visibility of the pickup widget as OnRep_Overlapping Weapon in BlasterCharacter.
void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// last weapon
	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon; 
	// manually call
	if (IsLocallyControlled()) {
		if (OverlappingWeapon) {
			// new weapon
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return Combat && Combat->EquippedWeapon ? true : false;
}

bool ABlasterCharacter::IsAiming()
{
	return Combat && Combat->bIsAming;
}

TObjectPtr<AWeapon> ABlasterCharacter::GetEquippedWeapon() const
{
	if (Combat) {
		return Combat->EquippedWeapon;
	}
	return nullptr;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (!Combat) {
		return {};
	}
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombateState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

