// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "Flag.generated.h"

class UStaticMeshComponent;
/**
 * 
 */
UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()
public:
	AFlag();

	FORCEINLINE FTransform GetInitialLocation() const { return InitialTransform; }
protected:
	virtual void BeginPlay() override;
	virtual void OnInitial() override;
	virtual void OnEquipped() override;
	virtual void OnDropped() override;
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> FlagMesh;

	FTransform InitialTransform;
};
