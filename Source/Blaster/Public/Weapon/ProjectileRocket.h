// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UAudioComponent;
class USoundAttenuation;
class URocketMovementComponent;
/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();
	virtual void Destroyed() override;
protected:
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
private:

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> RocketLoop;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAudioComponent> RocketLoopComponent;

	// For dynamically spawn
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundAttenuation> LoopingSoundAttenuation;
};
