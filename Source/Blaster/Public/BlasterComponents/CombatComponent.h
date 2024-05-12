// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/public/HUD/BlasterHUD.h"
#include "Blaster/Public/BlasterTypes/WeaponTypes.h"
#include "Blaster/Public/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class AWeapon;
class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterHUD;
class AProjectile;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	friend class ABlasterCharacter;

	/*operator UObject() { return static_cast<UObject*>(this); }*/

	// Sets default values for this component's properties
	UCombatComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* Weapon);
	void DropTheFlag();
	void EquipPrimaryWeapon(AWeapon* Weapon);
	void EquipSecondaryWeapon(AWeapon* Weapon);
	void SwapWeapons();

	void PlayEquipWeaponSound(AWeapon* Weapon);

	void UpdateCarriedAmmo();

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachWeaponToLeftHand(AWeapon* Weapon);
	//void AttachFlagToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);

	void FireButtonPressed(bool bPressed);

	void Reload();
	void ReloadEmptyWeapon();
	// on all machines
	void HandleReload();
	int32 AmountToReload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	void ShowAttachedGrenade(bool bShowGrenade);

	UFUNCTION(BlueprintCallable)
	void FinishSwapWeapon();

	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapon();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	bool bLocallyReloading = false;

	bool bLocallySwapping = false;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_Aiming();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	UFUNCTION()
	void OnRep_Flag();

	// replicate_notify from server to client can only have parameter that is the last one before rep.
	// howerver, RPC from client to server can have parameters.
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetHitTarget(FVector Target);

	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	void HandleThrowGrenade();
private:
	TObjectPtr<ABlasterCharacter> Character;
	TObjectPtr<ABlasterPlayerController> PlayerController;
	TObjectPtr<ABlasterHUD> HUD;
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Flag)
	TObjectPtr<AWeapon> Flag;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bIsAimng;

	bool bAimingButtonPressed;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	// HitTarget
	UPROPERTY(ReplicatedUsing = OnRep_HitTarget)
	FVector HitTarget;

	FVector LocalHitTarget;

	UFUNCTION()
	void OnRep_HitTarget();

	FHUDPackage HUDPackage;

	/** 
	*	HUD and crosshairs
	*/
	UPROPERTY(EditAnywhere)
	float CrosshairVelocityFactor;
	UPROPERTY(EditAnywhere)
	float CrosshairInAirFactor;
	UPROPERTY(EditAnywhere)
	float CrosshairAimFactor;
	UPROPERTY(EditAnywhere)
	float CrosshairShootingFactor;

	/** 
	*	Aiming and FOV
	*/

	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.0f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 30.0f;

	void InterpFOV(float DeltaTime);

	/** 
	*	Automatic Fire
	*/
	FTimerHandle FireTimer;
	bool bCanFire = true;

	bool CanFire();

	void StartFireTimer();
	void FireTimerFinished();

	// Carried ammo for the currently-equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 300;

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 120;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 5;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 36;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 160;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 36;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 36;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeAmmo = 36;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 5;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> GrenadeClass;

	UFUNCTION()
	void OnRep_Grenades();

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedGrenades = 5;

	void UpdateHUDGrenades();

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	void SetCombatState(ECombatState State);
	void OnCombatStateSet();

	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY(ReplicatedUsing = OnRep_HoldingTheFlag)
	bool bHoldingTheFlag = false;

	UFUNCTION()
	void OnRep_HoldingTheFlag();
public:	

	void UpdateAmmoValues();

	void UpdateShotgunAmmoValues();

	FORCEINLINE int32 GetGrenades() const { return Grenades; }

	bool ShouldSwapWeapons();
};
