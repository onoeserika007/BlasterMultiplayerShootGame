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
#include "Kismet/GameplayStatics.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"
#include "Components/BoxComponent.h"
#include "Blaster/Public/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Blaster.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Public/GameState/BlasterGameState.h"
#include "Blaster/Public/PlayerStart/TeamPlayerStart.h"

#define SETUP_HITBOX(boxName, soketName) \
    boxName = CreateDefaultSubobject<UBoxComponent>(TEXT(#boxName)); \
    boxName->SetupAttachment(GetMesh(), FName(#soketName)); \
    boxName->SetCollisionEnabled(ECollisionEnabled::NoCollision); \
	boxName->SetCollisionObjectType(ECC_HitBox); \
	boxName->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); \
	boxName->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block); \
	HitCollisionBoxes.Add(FName(#boxName), boxName);

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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));
	LagCompensation->SetIsReplicated(true);

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), TEXT("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	/**
	*	Hit boxes for server-side rewind
	*/
	SETUP_HITBOX(head, head);
	SETUP_HITBOX(pelvis, pelvis);
	SETUP_HITBOX(spine_02, spine_02);
	SETUP_HITBOX(spine_03, spine_03);
	SETUP_HITBOX(upperarm_l, upperarm_l);
	SETUP_HITBOX(upperarm_r, upperarm_r);
	SETUP_HITBOX(lowerarm_l, lowerarm_l);
	SETUP_HITBOX(lowerarm_r, lowerarm_r);
	SETUP_HITBOX(hand_l, hand_l);
	SETUP_HITBOX(hand_r, hand_r);
	SETUP_HITBOX(backpack, backpack);
	SETUP_HITBOX(blanket, backpack);
	SETUP_HITBOX(thigh_l, thigh_l);
	SETUP_HITBOX(thigh_r, thigh_r);
	SETUP_HITBOX(calf_l, calf_l);
	SETUP_HITBOX(calf_r, calf_r);
	SETUP_HITBOX(foot_l, foot_l);
	SETUP_HITBOX(foot_r, foot_r);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABlasterCharacter, OverlappingWeapon);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat) {
		Combat->Character = this;
	}

	if (Buff) {
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJumpVeclocity(GetCharacterMovement()->JumpZVelocity);
	}

	if (LagCompensation) {
		LagCompensation->Character = this;
		if (Controller) {
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
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
	if (AnimInstance && ReloadMontage && Combat && Combat->EquippedWeapon) {
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName = FName("Rifle");
		switch (Combat->EquippedWeapon->GetWeaponType()) {
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Sniper");
			break;
		case EWeaponType::EWT_GranadeLauncher:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage) {
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
		FName SectionName = FName("Default");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlaySwapWeaponMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance &&SwapWeaponMontage) {
		AnimInstance->Montage_Play(SwapWeaponMontage);
		FName SectionName = FName("Default");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

// on server
void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	if (HasAuthority()) {
		if (Combat) {
			DropOrDestroyWeapon(Combat->EquippedWeapon);
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
			DropOrDestroyWeapon(Combat->Flag);
		}
		MulticastElim(bPlayerLeftGame);
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon) {
		if (Weapon->bDestroyWeapon) {
			Weapon->Destroy();
		}
		else {
			Weapon->Dropped();
		}
	}
}

void ABlasterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && BlasterPlayerState && BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam) {
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts) {
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == BlasterPlayerState->GetTeam()) {
				TeamPlayerStarts.Add(TeamStart);
			}
		}

		if (TeamPlayerStarts.Num()) {
			int32 Selection = FMath::RandRange(0, TeamPlayerStarts.Num() - 1);
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[Selection];
			SetActorLocationAndRotation(
				ChosenPlayerStart->GetActorLocation(),
				ChosenPlayerStart->GetActorRotation()
			);
		}
		else {
			if (GEngine) {
				GEngine->AddOnScreenDebugMessage(
					-1,
					15,
					FColor::Red,
					FString("Respawning: No Player Start found!")
				);
			}
		}
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (LastShield > Shield) {
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	// however this is only valid on server, we need do this in destroyed in BlasterCharacter.
	//if (ElimBotComponent) {
	//	ElimBotComponent->DestroyComponent();
	//}

	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	if (BlasterGameMode && !bLeftGame) {
		BlasterGameMode->RequestRespawn(this, Controller);
	}


	if (bLeftGame && IsLocallyControlled()) {
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	ABlasterPlayerState* LeavingPlayerState = GetPlayerState<ABlasterPlayerState>();
	if (BlasterGameMode && BlasterPlayerState) {
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
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
	//if (BlasterPC) {
	//	DisableInput(BlasterPC);
	//}
	bDisableGameplay = true;
	if (Combat) {
		Combat->FireButtonPressed(false);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	bool bHideSniperScope = IsLocallyControlled() && Combat && Combat->bIsAimng && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope) {
		ShowSniperScopeWidget(false);
	}

	if (CrownComponent) {
		CrownComponent->DestroyComponent();
		CrownComponent = nullptr;
	}

	// we've moved the timer to the multicast, now we can perform our IsLocallyControlled check safely.
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ThisClass::ElimTimerFinished,
		RespawnDelay
	);
}

// This callback will only executed on server, so handle client's hit react in Shield and Health Rep.
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	if (bElimmed || !BlasterGameMode) return;

	Damage = BlasterGameMode->CalculateDamage(InstigatedBy, Controller, Damage);

	float DamageToHealth = FMath::Clamp(Damage - Shield, 0, MaxHealth);
	Shield = FMath::Clamp(Shield - Damage, 0, MaxShield);
	Health = FMath::Clamp(Health - DamageToHealth, 0.0f, MaxHealth);
	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();
	if (Health == 0.0f) {
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

	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (bMatchNotInProgress && Combat && Combat->EquippedWeapon) {
		Combat->EquippedWeapon->Destroy();
	}

	Super::Destroyed();
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	RotateInPlace(DeltaTime);

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
			EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Canceled, this, &ThisClass::AimCanceled);
		}

		if (IA_Fire) {
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Started, this, &ThisClass::Fire);
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Completed, this, &ThisClass::FireEnd);
		}

		if (IA_Reload) {
			EnhancedInputComponent->BindAction(IA_Reload, ETriggerEvent::Started, this, &ThisClass::Reload);
		}

		if (IA_Grenade) {
			EnhancedInputComponent->BindAction(IA_Grenade, ETriggerEvent::Started, this, &ThisClass::ThrowGrenade);
		}
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// this is needed since when OnPossess in Controller is called the HUD maybe not ready.
	// To sum up, Beginplay in Character is earlier than OnPossess in Controller, so these may be not working
	UpdateHUDHealth();
	UpdateHUDShield();

	if (HasAuthority()) {
		// onserver
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}

	if (AttachedGrenade) {
		AttachedGrenade->SetVisibility(false);
	}
	
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPC = BlasterPC == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPC;
	if (BlasterPC) {
		BlasterPC->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPC = BlasterPC == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPC;
	if (BlasterPC) {
		BlasterPC->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPC = BlasterPC == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPC;
	if (BlasterPC && Combat && Combat->EquippedWeapon) {
		BlasterPC->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPC->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !bElimmed && DefaultWeaponClass) {
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		if (StartingWeapon) {
			StartingWeapon->bDestroyWeapon = true;
		}
		if (Combat) {
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr) {
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState) {
			BlasterPlayerState->AddToScore(0.0f);
			BlasterPlayerState->AddToDefeats(0);
			SetTeamColor(BlasterPlayerState->GetTeam());
			SetSpawnPoint();

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState)) {
				MulticastGainedTheLead();
			}
		}
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (IsHoldingTheFlag()) {
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
		return;
	}

	if (Combat && Combat->EquippedWeapon) {
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}

	if (bDisableGameplay ) {
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

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
}

void ABlasterCharacter::Move(const FInputActionInstance& Instance)
{
	if (bDisableGameplay) return;
	const auto& values = Instance.GetValue().Get<FVector2D>();

	FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	FVector rightVec = UKismetMathLibrary::GetRightVector(YawRotation);
	FVector forwardVec = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(rightVec, values.X);
	AddMovementInput(forwardVec, values.Y);
}

void ABlasterCharacter::Look(const FInputActionInstance& Instance)
{
	const auto& values = Instance.GetValue().Get<FVector2D>();

	AddControllerPitchInput(-values.Y);
	AddControllerYawInput(values.X);
}

void ABlasterCharacter::Jump()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (bIsPersitentCrouching) {

	}
	Super::Jump();
	UnCrouch();
}

void ABlasterCharacter::Equip()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (Combat) {
		// do this locally is uneccessary, server will handle this, but do it locally for low delay
		//Combat->EquipWeapon(OverlappingWeapon);
		if (!OverlappingWeapon && Combat->ShouldSwapWeapons() && Combat->CombatState == ECombatState::ECS_Unoccupied) {
			Combat->SetCombatState(ECombatState::ECS_SwappingWeapons);
			ServerSwap();
		}
		
		if (OverlappingWeapon && Combat->CombatState == ECombatState::ECS_Unoccupied) {
			ServerEuip();
		}
	}
}

void ABlasterCharacter::ServerSwap_Implementation()
{
	if (Combat) {
		if (Combat->ShouldSwapWeapons()) {
			UE_LOG(LogTemp, Display, TEXT("ServerSwap_Implementation Called."));
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::ServerEuip_Implementation()
{
	if (Combat) {
		if (OverlappingWeapon) {
			Combat->EquipWeapon(OverlappingWeapon);
		}
	}
}

void ABlasterCharacter::CrouchCompleted()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (bIsCrouched) {
		Super::UnCrouch();
	}
	else {
		Super::Crouch();
	}
}

void ABlasterCharacter::CrouchOngoing()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	bIsPersitentCrouching = true;
	Super::Crouch();
}

void ABlasterCharacter::CrouchCanceled()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	bIsPersitentCrouching = false;
	Super::UnCrouch();
}

void ABlasterCharacter::AimTriggered()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimCanceled()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::Fire()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireEnd()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
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
	if (IsHoldingTheFlag()) return; 
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowGrenade()
{
	if (IsHoldingTheFlag()) return;
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::DropTheFlag()
{
	if (Combat) {
		Combat->DropTheFlag();
		OverlappingWeapon = nullptr;
	}
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem) {
		if (CrownComponent == nullptr) {
			CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				CrownSystem,
				GetCapsuleComponent(),
				FName(),
				GetActorLocation() + FVector(0.0f, 0.0f, 110.0f),
				GetActorRotation(),
				EAttachLocation::KeepWorldPosition,
				false
			);
		}

		if (CrownComponent) {
			CrownComponent->Activate();
		}
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent) {
		CrownComponent->DestroyComponent();
		CrownComponent = nullptr;
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

//void ABlasterCharacter::MulticastHit_Implementation()
//{
//	PlayHitReactMontage();
//}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (IsLocallyControlled()) {
		if (FollowCamera) {
			if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold) {
				GetMesh()->SetVisibility(false);
				if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
					Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
				}

				if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh()) {
					Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
				}
			}
			else {
				GetMesh()->SetVisibility(true);
				if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
					Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
				}

				if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh()) {
					Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
				}
			}
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (LastHealth > Health) {
		PlayHitReactMontage();
	}
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

bool ABlasterCharacter::IsWeaponEquipped()
{
	return Combat && Combat->EquippedWeapon ? true : false;
}

bool ABlasterCharacter::IsAiming()
{
	return Combat && Combat->bIsAimng;
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

void ABlasterCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (Combat) {
		Combat->bHoldingTheFlag = bHolding;
	}
}

// cannot inline, because the definition of Combat is used

bool ABlasterCharacter::IsLocallyReloading() const { return Combat ? Combat->bLocallyReloading : false; }

bool ABlasterCharacter::IsLocallySwapping() const { return Combat ? Combat->bLocallySwapping : false; }

void ABlasterCharacter::SetTeamColor(ETeam ColorToSet)
{
	if (!GetMesh()) return;
	switch (ColorToSet)
	{
	case ETeam::ET_RedTeam:
		if (RedMaterial && RedDissolveMaterialInstance) {
			GetMesh()->SetMaterial(0, RedMaterial);
			DissolveMaterialInstance = RedDissolveMaterialInstance;
		}
		break;
	case ETeam::ET_BlueTeam:
		if (BlueMaterial && BlueDissolveMaterialInstance) {
			GetMesh()->SetMaterial(0, BlueMaterial);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
		}
		break;
	case ETeam::ET_NoTeam:
		if (OriginalMaterial && OriginalDissolveMaterialInstance) {
			GetMesh()->SetMaterial(0, OriginalMaterial);
			DissolveMaterialInstance = OriginalDissolveMaterialInstance;
		}
		break;
	case ETeam::ET_MAX:
		break;
	default:
		break;
	}
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if (!Combat) return false;
	return Combat->bHoldingTheFlag;
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (!BlasterPlayerState) {
		return ETeam::ET_NoTeam;
	}
	return BlasterPlayerState->GetTeam();
}



