// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;
class USoundCue;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

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

protected:
	UPROPERTY(EditAnywhere)
	float BaseTurnRate = 45.0f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> PickupEffectComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> PickupEffect;

	// to delay OnOverlap callback to get time for OnDestroy callback to get bound.
	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
	void BindOverlapTimerFinished();

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> PickupSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

};
