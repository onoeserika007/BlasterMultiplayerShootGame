// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SpeedPickup.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"

ASpeedPickup::ASpeedPickup()
{
}

void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		if (UBuffComponent* Buff = BlasterCharacter->GetBuffComponent()) {
			Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
			Destroy();
		}
	}
}
