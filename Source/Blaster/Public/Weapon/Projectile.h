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

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	// functions
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	// exploit the destory func to broadcast
	/** Called when this actor is explicitly being destroyed during gameplay or in the editor, not called during level streaming or gameplay ending */
	virtual void Destroyed() override;

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
	float Damage = 20.0f;

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

// members
private:


	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;

	//UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent> TracerComponent;
public:	

};
