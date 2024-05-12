// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"
#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Blaster/Public/HUD/BlasterHUD.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Blaster/Public/BlasterTypes/CombatState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Blaster/Public/Charactor/BlasterAnimInstance.h"
#include "Blaster/Public/Weapon/Projectile.h"
#include "Blaster/Public/Weapon/Shotgun.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	BaseWalkSpeed = 600.0f;
	AimWalkSpeed = 415.0f;
}

// overlap only happens on server
void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (Character) {
		UE_LOG(LogTemp, Warning, TEXT("Pickingup Ammo!"));
		if (CarriedAmmoMap.Contains(WeaponType)) {
			CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
			UpdateCarriedAmmo();
		}

		// Histuyou nai desyou.
		if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType) {
			ReloadEmptyWeapon();
		}
	}
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetMainCamera()) {
			DefaultFOV = Character->GetMainCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority()) {
			InitializeCarriedAmmo();
		}
	}


}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (Character && Character->IsLocallyControlled()) {
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character && Character->Controller) {
		if (!PlayerController) {
			PlayerController = Cast<ABlasterPlayerController>(Character->Controller);
		}

		if (PlayerController) {
			if (!HUD) {
				HUD = Cast<ABlasterHUD>(PlayerController->GetHUD());
			}

			if (HUD) {
				bool bIsInSniperScope = false;
				if (EquippedWeapon) {
					bIsInSniperScope = bIsAimng && (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle);
				}

				if (EquippedWeapon && !bIsInSniperScope) {
					HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
					HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
					HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
					HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
					HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				}
				else {
					HUDPackage.CrosshairsCenter = nullptr;
					HUDPackage.CrosshairsLeft = nullptr;
					HUDPackage.CrosshairsRight = nullptr;
					HUDPackage.CrosshairsTop = nullptr;
					HUDPackage.CrosshairsBottom = nullptr;
				}

				// calc crosshair spread
				FVector2D WalkSpeedRange(0.0f, Character->GetCharacterMovement()->MaxWalkSpeed);
				FVector2D VelocityMultiplierRange(0.0f, 1.0f);
				float Velocity = Character->GetVelocity().Size2D();
				CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity);

				if (Character->GetCharacterMovement()->IsFalling()) {
					CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
				}
				else {
					CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 1.0f, DeltaTime, 30.0f);
				}

				if (bIsAimng) {
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.0f);
				}
				else {
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 1.0f, DeltaTime, 30.0f);
				}

				CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 1.0f, DeltaTime, 2.5f);

				HUDPackage.CrosshairSpread = (0.25f + CrosshairVelocityFactor) * CrosshairInAirFactor * CrosshairAimFactor * CrosshairShootingFactor;
				HUD->SetHUDPackage(HUDPackage);
			}
		}
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && Grenades > 0) {
		ServerThrowGrenade();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (CombatState == ECombatState::ECS_Unoccupied && Grenades > 0) {
		// CombatState = ECombatState::ECS_ThrowingGrenade;
		SetCombatState(ECombatState::ECS_ThrowingGrenade);
		// HandleThrowGrenade();
		// seting grenade nums is not a commom operation.
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxCarriedGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::HandleThrowGrenade()
{
	if (Character && EquippedWeapon) {
		Character->PlayThrowGrenadeMontage();
		ShowAttachedGrenade(true);
		//AttachActorToLeftHand(EquippedWeapon);
		AttachWeaponToLeftHand(EquippedWeapon);
	}
}

void UCombatComponent::OnRep_HitTarget()
{
	if (Character && Character->IsLocallyControlled()) {
		HitTarget = LocalHitTarget;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon) {
		if (bIsAimng) {
			CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
		}
		else {
			CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
		}
	}

	if (Character && Character->GetMainCamera()) {
		Character->GetMainCamera()->SetFieldOfView(CurrentFOV);
	}
}

bool UCombatComponent::CanFire()
{
	if (!EquippedWeapon) return false;
	bool bRet = bCanFire
		&& !EquippedWeapon->IsEmpty()
		&& ((CombatState == ECombatState::ECS_Unoccupied && !bLocallyReloading) || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun);
	if (!bRet) {
		UE_LOG(LogTemp, Warning, TEXT("CanFire CheckFailed"));
		UE_LOG(LogTemp, Warning, TEXT("bCanFire %d"), bCanFire);
		UE_LOG(LogTemp, Warning, TEXT("EquippedWeapon->IsEmpty() %d"), EquippedWeapon->IsEmpty());
		UE_LOG(LogTemp, Warning, TEXT("CombatState == ECombatState::ECS_Unoccupied %d"), CombatState == ECombatState::ECS_Unoccupied);
		UE_LOG(LogTemp, Warning, TEXT("bLocallyReloading %d"), bLocallyReloading);
	}
	return bRet;
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon || !Character) {
		return;
	}
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&ThisClass::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (!EquippedWeapon || !Character) {
		return;
	}

	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bIsAutomatic) {
		Fire();
	}

	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	PlayerController = !PlayerController ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController) {
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}

	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading
		&& EquippedWeapon
		&& EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun
		&& CarriedAmmo == 0;
	if (bJumpToShotgunEnd) {
		// Jump to ShotgunEnd Section
		// Check on client is performed in OnRep_Ammo in Weapon.cpp
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	if (!Character) return;
	PlayerController = !PlayerController ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController) {
		PlayerController->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GranadeLauncher, StartingGrenadeAmmo);
}

void UCombatComponent::SetAiming(bool bAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	if (Character->IsLocallyControlled()) {
		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle && bAiming != bIsAimng) {
			Character->ShowSniperScopeWidget(bAiming);
		}
		ServerSetAiming(bAiming);
		bIsAimng = bAiming;
		bAimingButtonPressed = bAiming;
	}
}

// onServer
void UCombatComponent::ServerSetAiming_Implementation(bool bAiming)
{
	this->bIsAimng = bAiming;
	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_Aiming()
{
	// Local pressed take the charge
	if (Character && Character->IsLocallyControlled()) {
		bIsAimng = bAimingButtonPressed;
	}
}

void UCombatComponent::ServerSetHitTarget_Implementation(FVector Target)
{
	this->HitTarget = Target;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed) {
		// note that trace is done locally, since player can cheat.
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire()) {
		// if shotgun interupt reloading, the LocallyReloading bool should be set.
		if (EquippedWeapon) {
			CrosshairShootingFactor = 5.5f;
			bCanFire = false;
			switch (EquippedWeapon->FireType) {
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			default:
				break;
			}
			StartFireTimer();
		}
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon) {
		FVector HitEnd = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		LocalFire(HitEnd);
		ServerFire(HitEnd, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon) {
		FVector HitEnd = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		LocalFire(HitEnd);
		ServerFire(HitEnd, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun) {
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		LocalShotgunFire(HitTargets);
		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
	}
}

// for RPC, suffix implementation is needed.
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	// why we don't just notify other clients by replicate? For automatic weapon bool will not change, and this bool will not be replicated.
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon) {
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// Multicast_Fire wouldn't fire twice on cleint and server who calls fire.
	if (Character) {
		if (Character->IsLocallyControlled()) return;
		LocalFire(TraceHitTarget);
	}
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	if (EquippedWeapon) {
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character) {
		if (Character->IsLocallyControlled()) return;
		LocalShotgunFire(TraceHitTargets);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon) {
		if (Character) {
			Character->PlayFireMontage(bIsAimng);
			// since simulate_proxy don't have a viewport, the trace can only be done locally.
			//EquippedWeapon->Fire(HitTarget);
			EquippedWeapon->Fire(TraceHitTarget);
			// set combatState back to Unoccupied in case that shotgun fire when reloading
			//CombatState = ECombatState::ECS_Unoccupied;
			SetCombatState(ECombatState::ECS_Unoccupied);
		}
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (EquippedWeapon) {
		if (Character) {
			Character->PlayFireMontage(bIsAimng);
			AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
			if (Shotgun) {
				Shotgun->FireShotgun(TraceHitTargets);
			}
			//CombatState = ECombatState::ECS_Unoccupied;
			SetCombatState(ECombatState::ECS_Unoccupied);
		}
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld) {
		FVector Start = CrosshairWorldPosition;
		if (Character) {
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 50.0f);
			//DrawDebugSphere(GetWorld(), Start, 16.0f, 12, FColor::Yellow, false);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit) {
			TraceHitResult.ImpactPoint = End;
		}
		else {
			//DrawDebugSphere(
			//	GetWorld(),
			//	TraceHitResult.ImpactPoint,
			//	12.f,
			//	12,
			//	FColor::Red
			//);
		}
		//HitTarget = TraceHitResult.ImpactPoint;
		// sync hit target
		HitTarget = TraceHitResult.ImpactPoint;
		LocalHitTarget = HitTarget;
		ServerSetHitTarget(TraceHitResult.ImpactPoint);

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>()) {
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else {
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

// replicate only from server to client.
// if we want to sync a state from a client, we should combine RPC and replication.
void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME(ABlasterCharacter, OverlappingWeapon);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAimng);
	DOREPLIFETIME(UCombatComponent, HitTarget);
	//DOREPLIFETIME(UCombatComponent, CarriedAmmo);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
	DOREPLIFETIME(UCombatComponent, bHoldingTheFlag);
	DOREPLIFETIME(UCombatComponent, Flag);
}

// called by serverEuip in BlasterCharacter, only on server.
void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
	// only on server
	if (!Weapon || !Character || Weapon == EquippedWeapon || CombatState != ECombatState::ECS_Unoccupied) {
		return;
	}

	if (Weapon->GetWeaponType() == EWeaponType::EWT_Flag) {
		Flag = Weapon;
		// maybe this function only works locally
		Character->Crouch();
		bHoldingTheFlag = true;
		// CharacterMovement is handled in RotateInPlace in BalsterCharacter.
		Weapon->SetWeaponState(EWeaponState::EWS_Equipped);
		Weapon->SetOwner(Character);
		AttachWeaponToLeftHand(Weapon);
	}
	else {
		if (EquippedWeapon && !SecondaryWeapon) {
			EquipSecondaryWeapon(Weapon);
		}
		else {
			EquipPrimaryWeapon(Weapon);
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::DropTheFlag()
{
	if (Flag) {
		Flag->Dropped();
		Flag->SetWeaponState(EWeaponState::EWS_Initial);
		Flag = nullptr;
		Character->UnCrouch();
		bHoldingTheFlag = false;
	}
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* Weapon)
{
	if (!Weapon) return;
	// drop old weapon
	if (EquippedWeapon) {
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = Weapon;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	// this was also replicated, due to its owner property has a OnRep_Owner callback
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDWeaponAmmo(); // in client, the HUD will be set after owner rep in OnRep_Owner in weapon.cpp.

	UpdateCarriedAmmo();

	PlayEquipWeaponSound(Weapon);

	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* Weapon)
{
	if (!Weapon) return;
	SecondaryWeapon = Weapon;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	SecondaryWeapon->SetOwner(Character);
	AttachActorToBackpack(SecondaryWeapon);
	PlayEquipWeaponSound(SecondaryWeapon);
	//SecondaryWeapon->EnableCustomDepth(true);
}

// only on server
void UCombatComponent::SwapWeapons()
{
	//if (CombatState != ECombatState::ECS_Unoccupied) return;
	// Now that you checked in BlasterCharacter::Equip() the combat state, you shouldn't check twice.
	// Or you can move that two function in CombatComponent.

	// in case player on server already set ECS_SwappingWeapons
	if (Character && !Character->IsLocallyControlled()) {
		SetCombatState(ECombatState::ECS_SwappingWeapons);
	}

	UE_LOG(LogTemp, Display, TEXT("SwapWeapons Called"));
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* Weapon)
{
	if (Weapon && Weapon->EquipSound && Character) {
		UGameplayStatics::PlaySoundAtLocation(
			this,
			//static_cast<USoundBase*>(EquippedWeapon->EquipSound),
			// cast error here! if you don't include the defination, how can the compiler know it's base class?
			Weapon->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (!Character || !EquippedWeapon) return;
	// carried ammo
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	PlayerController = !PlayerController ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController) {
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	//// at least this can't
	//EquippedWeapon->ShowPickupWidget(false);
	////EquippedWeapon->AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character && Character->GetMesh() && ActorToAttach) {
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		// this can be propogated to client
		if (HandSocket) {
			HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
		}
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character && Character->GetMesh() && ActorToAttach) {
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("LeftHandSocket"));
		// this can be propogated to client
		if (HandSocket) {
			HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
		}
	}
}

void UCombatComponent::AttachWeaponToLeftHand(AWeapon* Weapon)
{
	if (Character && Character->GetMesh() && Weapon) {
		FName SocketName;
		switch (Weapon->GetWeaponType()) {
		case EWeaponType::EWT_Pistol:
		case EWeaponType::EWT_SubmachineGun:
			SocketName = FName("LeftHandPistolSocket");
			break;
		case EWeaponType::EWT_Flag:
			SocketName = FName("FlagSocket");
			break;
		default:
			SocketName = FName("LeftHandRifleSocket");
			break;
		}
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
		// this can be propogated to client
		if (HandSocket) {
			HandSocket->AttachActor(Weapon, Character->GetMesh());
		}
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character && Character->GetMesh() && ActorToAttach) {
		const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
		// this can be propogated to client
		if (BackpackSocket) {
			BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
		}
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character && ECombatState::ECS_SwappingWeapons != CombatState) {
		// we can't make sure if the weapon or character was copied earlier in network, so do binding redundantly.
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		PlayEquipWeaponSound(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		//EquippedWeapon->SetHUDWeaponAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character && CombatState != ECombatState::ECS_SwappingWeapons) {
		// we can't make sure if the weapon or character was copied earlier in network, so do binding redundantly.
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
		//SecondaryWeapon->EnableCustomDepth(true);
	}
}

void UCombatComponent::OnRep_Flag()
{
	if (Flag && Character && CombatState != ECombatState::ECS_SwappingWeapons) {
		// we can't make sure if the weapon or character was copied earlier in network, so do binding redundantly.
		Flag->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToBackpack(Flag);
		PlayEquipWeaponSound(SecondaryWeapon);
		//SecondaryWeapon->EnableCustomDepth(true);
	}
}

void UCombatComponent::Reload()
{
	if (!EquippedWeapon) return;
	// check to prevent spam
	// CombatState may has lag, use bLocallyReloading == false checking additionally
	if (CarriedAmmo > 0 && ECombatState::ECS_Unoccupied == CombatState && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() && !bLocallyReloading) {
		//HandleReload();
		SetCombatState(ECombatState::ECS_Reloading);
		ServerReload();
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && CarriedAmmo > 0) {
		Reload();
	}
}

// ServerReload
void UCombatComponent::ServerReload_Implementation()
{
	if (Character && EquippedWeapon) {

		PlayerController = !PlayerController ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController) {
			PlayerController->SetHUDWeaponAmmo(0);
		}

		//CombatState = ECombatState::ECS_Reloading;
		if (!Character->IsLocallyControlled()) {
			//HandleReload();
			SetCombatState(ECombatState::ECS_Reloading);
		}
	}
}

void UCombatComponent::HandleReload()
{
	if (Character) {
		Character->PlayReloadMontage();
	}

	bLocallyReloading = true;

	if (bIsAimng) {
		SetAiming(false);
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	// Inredibly RoomInMag comes to 0 after it is assigned to 25 in SMG case!!!!
	const int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		const int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		const int32 Least = FMath::Min(RoomInMag, AmountCarried);
		// in case ammo > mag
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::FinishReloading()
{
	if (Character) {
		bLocallyReloading = false;
		if (Character->HasAuthority()) {
			//CombatState = ECombatState::ECS_Unoccupied;
			SetCombatState(ECombatState::ECS_Unoccupied);
			UpdateAmmoValues();
			// server
			if (bFireButtonPressed) {
				Fire();
			}
		}
	}
}

void UCombatComponent::FinishSwapWeapon()
{
	if (Character && Character->HasAuthority()) {
		SetCombatState(ECombatState::ECS_Unoccupied);
	}
	bLocallySwapping = false;
}

// happens on all machines
void UCombatComponent::FinishSwapAttachWeapon()
{
	UE_LOG(LogTemp, Display, TEXT("FinishSwapAttachWeapon Called"));
	// New EquippedWepaon
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDWeaponAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);
	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}

	// New SecondaryWeapon
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

void UCombatComponent::ShotgunShellReload()
{
	bLocallyReloading = true;
	// coz Ammo is replicated, here is only on server
	if (Character && Character->HasAuthority()) {
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::SetCombatState(ECombatState State)
{
	CombatState = State;
	OnCombatStateSet();
}

void UCombatComponent::OnCombatStateSet()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		// client
		if (bFireButtonPressed) {
			Fire();
		}
		bLocallyReloading = false;
		bLocallySwapping = false;
		break;
	case ECombatState::ECS_Reloading:
		//if (Character && Character->IsLocallyControlled()) break;
		HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		HandleThrowGrenade();
		break;
	case ECombatState::ECS_SwappingWeapons:
		if (Character) {
			Character->PlaySwapWeaponMontage();
		}
		bLocallySwapping = true;
		break;
	default:
		break;
	}
}

// reload dispatched by CombatState rep.
void UCombatComponent::OnRep_CombatState()
{
	OnCombatStateSet();
}

void UCombatComponent::OnRep_HoldingTheFlag()
{
	if (Character && Character->IsLocallyControlled()) {
		//if (bHoldingTheFlag) {
		//	Character->Crouch();
		//}
		//else {
		//	Character->UnCrouch();
		//}
		UE_LOG(LogTemp, Display, TEXT("Crouch on client called."));
		Character->Crouch();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character && EquippedWeapon) {
		int32 ReloadAmount = AmountToReload();
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
			CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
			CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		}
		// on server call SetHUDWeaponAmmo manually.
		EquippedWeapon->AddAmmo(ReloadAmount);

		// on server call SetHUDCarriedAmmo manually.
		// on client this will be handled by onrep ammo and carried ammo;
		PlayerController = !PlayerController ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController) {
			PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character && EquippedWeapon) {
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
			CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
			CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		}
		// on server call SetHUDWeaponAmmo manually.
		EquippedWeapon->AddAmmo(1);

		// on server call SetHUDCarriedAmmo manually.
		// on client this will be handled by onrep ammo and carried ammo;
		PlayerController = !PlayerController ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController) {
			PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
		}

		if (EquippedWeapon->IsFull() || CarriedAmmo == 0) {
			// Jump to ShotgunEnd Section
			// Check on client is performed in OnRep_Ammo in Weapon.cpp
			JumpToShotgunEnd();
		}
	}
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return EquippedWeapon && SecondaryWeapon;
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage()) {
		FName SectionName = FName("ShotgunEnd");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	if (Character->HasAuthority()) {
		//CombatState = ECombatState::ECS_Unoccupied;
		SetCombatState(ECombatState::ECS_Unoccupied);
	}
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (Character && Character->IsLocallyControlled()) {
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade()) {
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}



void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade()) {
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawmParams;
		SpawmParams.Owner = Character;
		SpawmParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World) {
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawmParams
			);
		}
	}
}

