// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/HealthPickup.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		if (UBuffComponent* Buff = BlasterCharacter->GetBuffComponent()) {
			Buff->Heal(HealAmount, HealingTime);
			Destroy();
		}
	}
}
