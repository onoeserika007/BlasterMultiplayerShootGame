// Fill out your copyright notice in the Description page of Project Settings.


#include "Charactor/BlasterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Public/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!BlasterCharacter) {
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (BlasterCharacter) {
		Speed = BlasterCharacter->GetVelocity().Size2D();
		bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

		bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
		bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
		EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
		bIsCrouched = BlasterCharacter->bIsCrouched;
		bIsAiming = BlasterCharacter->IsAiming();
		TurningInPlace = BlasterCharacter->GetTurningInplace();
		bIsLocallyControlled = BlasterCharacter->IsLocallyControlled();
		bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
		bElimmed = BlasterCharacter->IsElimmed();

		FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
		//UE_LOG(LogTemp, Warning, TEXT("AimRotation Yaw %f: "), AimRotation.Yaw);
		//UE_LOG(LogTemp, Warning, TEXT("MoveRotation Yaw %f: "), MovementRotation.Yaw);
		FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
		// don't use the interplation in the blend space, this magic interp function will use the shortest path to interp 180 and -180
		DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
		YawOffset = DeltaRotation.Yaw;

		CharacterRotationLastFrame = CharacterRotation;
		CharacterRotation = BlasterCharacter->GetActorRotation();
		const float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame).Yaw;
		const float Target = DeltaYaw / DeltaTime;
		const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
		Lean = FMath::Clamp(Interp, -90.f, 90.f);

		AO_Yaw = BlasterCharacter->GetAO_Yaw();
		AO_Pitch = BlasterCharacter->GetAO_Pitch();

		if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh()) {
			// lefthand
			LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
			FVector OutPosition;
			FRotator OutRotation;
			BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
			LeftHandTransform.SetLocation(OutPosition);
			LeftHandTransform.SetRotation(FQuat(OutRotation));

			// righthand x-axis is opposite to target
			// optional to restrict to locally controlled
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandLookAtTargetRotation = FMath::RInterpTo(RightHandLookAtTargetRotation, LookAtRotation, DeltaTime, 30.0f);

			// muzzle tip dir debug
			FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
			FVector MuzzleX{ FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X) };
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.0f, FColor::Red);
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Cyan);
		}

		bUseFABRIK = BlasterCharacter->GetCombateState() != ECombatState::ECS_Reloading;
		bUseAimOffsets = BlasterCharacter->GetCombateState() != ECombatState::ECS_Reloading;
		bTransformRightHand = BlasterCharacter->GetCombateState() != ECombatState::ECS_Reloading;
	}

}
