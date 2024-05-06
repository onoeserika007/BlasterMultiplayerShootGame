// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/Public/BlasterTypes/TurningInPlace.h"
#include "Blaster/Public/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/Public/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UWidgetComponent;
class AWeapon;
class UCombatComponent;
class UAnimMontage;
class ABlasterPlayerController;
class USoundCue;
class ABlasterPlayerState;
class UBuffComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Allow actors to initialize themselves on the C++ side after all of their components have been initialized, only called during gameplay */
	virtual void PostInitializeComponents() override;

	virtual void OnRep_ReplicatedMovement() override;

	void PlayFireMontage(bool bIsAiming);

	void PlayHitReactMontage();

	void PlayElimMontage();

	void PlayReloadMontage();

	void PlayThrowGrenadeMontage();

	void Elim();
	void DropOrDestroyWeapon(AWeapon* Weapon);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void UpdateHUDHealth();

	void UpdateHUDShield();

	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	// Poll for any relevant classes and initialize our HUD
	void PollInit();
	void RotateInPlace(float DeltaTime);
private:
	// inputs
	UPROPERTY(EditDefaultsOnly, Category = "EnhancedInput")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Equip;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<UInputAction> IA_SwapWeapons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Crouch;
	bool bIsPersitentCrouching = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Aim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Grenade;
public:	
	UFUNCTION()
	void Move(const FInputActionInstance& Instance);

	UFUNCTION()
	void Look(const FInputActionInstance& Instance);

	void Jump() override;

	UFUNCTION()
	void Equip();

	//UFUNCTION()
	//void TrySwapWeapons();

	UFUNCTION()
	void CrouchCompleted();

	UFUNCTION()
	void CrouchOngoing();

	UFUNCTION()
	void CrouchCanceled();

	UFUNCTION()
	void AimTriggered();

	UFUNCTION()
	void AimCanceled();

	UFUNCTION()
	void Fire();

	UFUNCTION()
	void FireEnd();

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	UFUNCTION()
	void Reload();

	UPROPERTY(Replicated)
	bool bDisableGameplay;

	UFUNCTION()
	void ThrowGrenade();
private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UWidgetComponent> OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBuffComponent> Buff;

	UFUNCTION(Server, Reliable)
	void ServerEuipped();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	*	Animation Montage
	*/

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;

	void HideCharacterIfCameraClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.0f;

	bool bRotateRootBone;
	float TurnThreshold = 1.57f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	/** 
	*	Player Health // since replication through PlayerState is slower than in Actor, Health will be set to here.
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.0f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/**
	*	Player Shield 
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 125.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 50.0f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	TObjectPtr<ABlasterPlayerController> BlasterPC;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float RespawnDelay = 3.0f;
	void ElimTimerFinished();

	/** 
	*	Dissolve Effect
	*/

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTimelineComponent> DissolveTimelineComponent;
	//UTimelineComponent* DissolveTimelineComponent;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> DissolveCurve;

	FOnTimelineFloat DissolveTrack;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// Dynamic instance that we change at runtime
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material.
	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	/*  
	* Elim Bot
	*/
	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UParticleSystem> ElimBotEffect;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UParticleSystemComponent> ElimBotComponent;

	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<USoundCue> ElimBotSound;

	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;

	/* 
	*	Grenade
	*/

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> AttachedGrenade;

	/*
	*	DefaultWeaponClass
	*/
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;
public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	TObjectPtr<AWeapon> GetEquippedWeapon() const;
	FORCEINLINE ETurningInPlace GetTurningInplace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetMainCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombateState() const;
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuffComponent() const { return Buff; }
	FORCEINLINE bool IsDisableGameplay() const { return bDisableGameplay; }

	// hit reaction
	//UFUNCTION(NetMulticast, Unreliable)
	//void MulticastHit();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
};
