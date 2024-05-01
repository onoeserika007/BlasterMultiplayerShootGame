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

	void Reload();
	// on all machines
	void HandleReload();
	int32 AmountToReload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	// replicate_notify from server to client can only have parameter that is the last one before rep.
	// howerver, RPC from client to server can have parameters.
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetHitTarget(FVector Target);

	void FireButtonPressed(bool bPressed);

	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();
private:
	TObjectPtr<ABlasterCharacter> Character;
	TObjectPtr<ABlasterPlayerController> PlayerController;
	TObjectPtr<ABlasterHUD> HUD;
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY(Replicated)
	bool bIsAming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	// HitTarget
	UPROPERTY(Replicated)
	FVector HitTarget;

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

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 90;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();
public:	

		
	void UpdateAmmoValues();

};
