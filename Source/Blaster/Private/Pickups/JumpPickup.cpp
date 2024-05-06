// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/JumpPickup.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		if (UBuffComponent* Buff = BlasterCharacter->GetBuffComponent()) {
			//Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
			Destroy();
		}
	}
}
