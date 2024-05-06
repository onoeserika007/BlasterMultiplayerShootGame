// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/AmmoPickup.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	UE_LOG(LogTemp, Warning, TEXT("Overlap!"));
	if (BlasterCharacter) {
		if (UCombatComponent* Combat = BlasterCharacter->GetCombatComponent()) {
			Combat->PickupAmmo(WeaponType, AmmoAmount);
			Destroy();
		}
	}
}

void AAmmoPickup::OnSphereEndOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex);
}
