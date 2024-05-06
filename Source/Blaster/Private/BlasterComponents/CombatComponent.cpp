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
					bIsInSniperScope = bIsAming && (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle);
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

				if (bIsAming) {
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
		CombatState = ECombatState::ECS_ThrowingGrenade;
		HandleThrowGrenade();
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

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon) {
		if (bIsAming) {
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
	return bCanFire && !EquippedWeapon->IsEmpty() && (CombatState == ECombatState::ECS_Unoccupied || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun);
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

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle && bIsAiming != this->bIsAming) {
		Character->ShowSniperScopeWidget(bIsAiming);
	}

	ServerSetAiming(bIsAiming);
}

// onServer
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	this->bIsAming = bIsAiming;
	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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
		if (EquippedWeapon) {
			CrosshairShootingFactor = 5.5f;
			bCanFire = false;
			ServerFire(HitTarget);
			StartFireTimer();
		}
	}
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon) {
		if (Character) {
			Character->PlayFireMontage(bIsAming);
			// since simulate_proxy don't have a viewport, the trace can only be done locally.
			//EquippedWeapon->Fire(HitTarget);
			EquippedWeapon->Fire(TraceHitTarget);
			// set combatState back to Unoccupied in case that shotgun fire when reloading
			CombatState = ECombatState::ECS_Unoccupied;
		}
	}
}

// for RPC, suffix implementation is needed.
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// why we don't just notify other clients by replicate? For automatic weapon bool will not change, and this bool will not be replicated.
	MulticastFire(TraceHitTarget);
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
			DrawDebugSphere(GetWorld(), Start, 16.0f, 12, FColor::Yellow, false);
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
			DrawDebugSphere(
				GetWorld(),
				TraceHitResult.ImpactPoint,
				12.f,
				12,
				FColor::Red
			);
		}
		//HitTarget = TraceHitResult.ImpactPoint;
		// sync hit target
		HitTarget = TraceHitResult.ImpactPoint;
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
	DOREPLIFETIME(UCombatComponent, bIsAming);
	DOREPLIFETIME(UCombatComponent, HitTarget);
	//DOREPLIFETIME(UCombatComponent, CarriedAmmo);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}

// called by serverEuip, only on server.
void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
	// only on server
	if (!Weapon || !Character || Weapon == EquippedWeapon || CombatState != ECombatState::ECS_Unoccupied) {
		return;
	}

	if (EquippedWeapon && !SecondaryWeapon) {
		EquipSecondaryWeapon(Weapon);
	}
	else {
		EquipPrimaryWeapon(Weapon);
	}

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
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

	PlayEquippWeaponSound(Weapon);

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
	PlayEquippWeaponSound(SecondaryWeapon);
	//SecondaryWeapon->EnableCustomDepth(true);
}

void UCombatComponent::SwapWeapons()
{
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	// New EquippedWepaon
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDWeaponAmmo();
	UpdateCarriedAmmo();
	PlayEquippWeaponSound(EquippedWeapon);
	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}

	// New SecondaryWeapon
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

void UCombatComponent::PlayEquippWeaponSound(AWeapon* Weapon)
{
	if (Weapon->EquipSound && Character && Weapon) {
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
	if (EquippedWeapon && Character) {
		// we can't make sure if the weapon or character was copied earlier in network, so do binding redundantly.
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		PlayEquippWeaponSound(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		//EquippedWeapon->SetHUDWeaponAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character) {
		// we can't make sure if the weapon or character was copied earlier in network, so do binding redundantly.
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquippWeaponSound(SecondaryWeapon);
		//SecondaryWeapon->EnableCustomDepth(true);
	}
}

void UCombatComponent::Reload()
{
	if (!EquippedWeapon) return;
	// check to prevent spam
	if (CarriedAmmo > 0 && ECombatState::ECS_Unoccupied == CombatState && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity()) {
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

		CombatState = ECombatState::ECS_Reloading;
		HandleReload();
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
	if (bIsAming) {
		SetAiming(false);
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		// in case ammo > mag
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::FinishReloading()
{
	if (Character) {
		if (Character->HasAuthority()) {
			CombatState = ECombatState::ECS_Unoccupied;
			UpdateAmmoValues();
			// server
			if (bFireButtonPressed) {
				Fire();
			}
		}
	}
}

void UCombatComponent::ShotgunShellReload()
{
	// coz Ammo is replicated, here is only on server
	if (Character && Character->HasAuthority()) {
		UpdateShotgunAmmoValues();
	}
}

// reload dispatched by CombatState rep.
void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		// client
		if (bFireButtonPressed) {
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		HandleThrowGrenade();
		break;
	default:
		break;
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
		CombatState = ECombatState::ECS_Unoccupied;
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

