// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USoundCue;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class AWeapon;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	// functions
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	// exploit the destory func to broadcast
	/** Called when this actor is explicitly being destroyed during gameplay or in the editor, not called during level streaming or gameplay ending */
	virtual void Destroyed() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticle;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;

	UFUNCTION()
	void SpawnTrailSystem();

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
	void StartDestroyTimer();
	void DestroyTimerFinished();

	UPROPERTY(EditDefaultsOnly)
	float DamageInnerRadius = 200.0f;
	UPROPERTY(EditDefaultsOnly)
	float DamageOuterRadius = 500.0f;
	UPROPERTY(EditDefaultsOnly)
	float MinimumDamage = 10.0f;
	void ExplodeDamage();

	//UFUNCTION(NetMulticast, Reliable)
	// reliable? maybe but unecessary, we will spawn on client rather than use replicates from server most time.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastOnDestroyed();

	void HandleDestroyed();

// members
private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;

	//UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent> TracerComponent;

	bool bHit = false;
public:	
	/** 
	*	Used with server-side rewind
	*/

	UPROPERTY(EditAnywhere)
	bool bUseServerSideRewind = false;

	FVector_NetQuantize TraceStart;
	// 2 decimal place of precision, more accureate than FVector_NetQuantize
	FVector_NetQuantize100 InitialVelocity;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000;

	// Only Set this for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;

	// Doesn't matter for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.0f;

	TObjectPtr<AWeapon> ShotWeapon = nullptr;

	FORCEINLINE bool IsReplicated() const { return bReplicates; }
};
