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

#define TRACE_LENGTH 800000.0f

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
				if (EquippedWeapon) {
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
	return bCanFire && !EquippedWeapon->IsEmpty() && CombatState == ECombatState::ECS_Unoccupied;
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
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
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
	DOREPLIFETIME(UCombatComponent, bIsAming);
	DOREPLIFETIME(UCombatComponent, HitTarget);
	//DOREPLIFETIME(UCombatComponent, CarriedAmmo);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

// called by serverEuip, only on server.
void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
	// only on server
	if (!Weapon || !Character) {
		return;
	}

	// drop old weapon
	if (EquippedWeapon) {
		EquippedWeapon->Dropped();
	}

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			2,
			FColor::Cyan,
			FString(TEXT("Euip Start."))
		);
	}

	EquippedWeapon = Weapon;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	// this can be propogated to client
	if (HandSocket) { 
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	// this was also replicated, due to its owner property has a OnRep_Owner callback
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDWeaponAmmo(); // in client, the HUD will be set after owner rep in OnRep_Owner in weapon.cpp.

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

	if (EquippedWeapon->EquipSound) {
		UGameplayStatics::PlaySoundAtLocation(
			this,
			//static_cast<USoundBase*>(EquippedWeapon->EquipSound),
			// 
			// cast error here! if you don't include the defination, how can the compiler know it's base class?
			//
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}

	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character) {
		// we can't make sure if the weapon or character was copied earlier in network, so do binding redundantly.
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		// this can be propogated to client
		if (HandSocket) {
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		if (EquippedWeapon->EquipSound) {
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EquipSound,
				Character->GetActorLocation()
			);
		}
	}
}

void UCombatComponent::Reload()
{
	if (!EquippedWeapon) return;
	// check to prevent spam
	if (CarriedAmmo > 0 && ECombatState::ECS_Reloading != CombatState && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity()) {
		ServerReload();
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
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
	default:
		break;
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

