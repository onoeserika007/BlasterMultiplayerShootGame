// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blaster/Public/BlasterTypes/WeaponTypes.h"
#include "Blaster/Public/BlasterTypes/Team.h"
#include "Weapon.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;
class ACasing;
class UTexture2D;
class ABlasterCharacter;
class ABlasterPlayerController;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponState : uint8 {
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8 {
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};



UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnInitial();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped(); 

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlapComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlapComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
		);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** 
	* OnRep functions
	*/
	UFUNCTION()
	void OnRep_WeaponState();
	virtual void OnRep_Owner() override;
	UFUNCTION()
	void OnRep_Ammo();
	void SetHUDWeaponAmmo();

	void ShowPickupWidget(bool bShowWidget);
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE TObjectPtr<USkeletalMeshComponent> GetWeaponMesh() const {
		return WeaponMesh;
	}
	void Fire();
	virtual void Fire(const FVector& HitTarget);
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	virtual void Dropped();
	bool IsEmpty();
	bool IsFull();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	void AddAmmo(int32 amount);
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotMagnification() const { return HeadShotMagnification; }
	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }
	FORCEINLINE ETeam GetTeam() const { return Team; }

	/**
	* Textures for the weapon crosshairs
	*/
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsBottom;

	/**
	*	Zoomed FOV while aiming
	*/
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	float ZoomedFOV = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	float ZoomInterpSpeed = 30.0f;

	/**
	*	Automatic Fire
	*/
	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bIsAutomatic = true;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> EquipSound;

	/*
	* Enable or disbale custom depth
	*/
	void EnableCustomDepth(bool bEnable);

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	/*
	*	Trace End with Scatter
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	FVector TraceEndWithScatter(const FVector& HitTarget);

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
protected:

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(EditAnywhere)
	float Damage = 12.0f;

	UPROPERTY(EditAnywhere)
	float HeadShotMagnification = 2.0f;

	UPROPERTY(EditAnywhere, Replicated)
	bool bUseServerSideRewind = false;
	TObjectPtr<ABlasterCharacter> BlasterOwnerCharacter;
	TObjectPtr<ABlasterPlayerController> BlasterOwnerController;
	FTransform RelativeTransformFromRootToMesh;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<ACasing> CasingClass;

	// count of Ammo remaining
	// UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	void SpendRound();

	// Client RPC Runs only on actor's owning client, so the ammo of the weapon for the second client won't be updated.
	//UFUNCTION(Client, Reliable)
	UFUNCTION(NetMulticast, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	//UFUNCTION(Client, Reliable)
	UFUNCTION(NetMulticast, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	// The number of unprocessed server requests for ammo
	// Increment in SpendRound, decremented in ClientUpdateAmmo
	int32 AmmoSequence = 0;

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere)
	ETeam Team = ETeam::ET_NoTeam;
};
