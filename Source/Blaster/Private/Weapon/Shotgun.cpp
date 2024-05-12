// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Public/BlasterComponents/LagCompensationComponent.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AWeapon::Fire(FVector());

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (InstigatorPawn == nullptr) return;
	AController* InstigatorController = InstigatorPawn->GetController();

	if (!HasAuthority() && !InstigatorController) UE_LOG(LogTemp, Warning, TEXT("Controller is invalid on simulatedProxy"));
	if (HasAuthority() && !InstigatorController) UE_LOG(LogTemp, Warning, TEXT("Controller is always valid on Autority"));

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket) {
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// From muzzle flash socket to hit location from TraceUnderCrosshairs
		FVector Start = SocketTransform.GetLocation();

		//TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, float> HitMap; // ptr -> damage
		for (int32 i = 0; i < TraceHitTargets.Num(); i++) {
			FHitResult FireHit;
			WeaponTraceHit(Start, TraceHitTargets[i], FireHit);

			ABlasterCharacter* OtherActor = Cast<ABlasterCharacter>(FireHit.GetActor());

			// Hit Character
			// There is no need for checking Autority when calculate hits. Otherwise client won't get right hits to send.
			if (OtherActor && InstigatorController) {
				const float Magnification = FireHit.BoneName.ToString() == FString("head") ? HeadShotMagnification : 1.0f;
				if (HitMap.Contains(OtherActor)) {
					HitMap[OtherActor] += Damage * Magnification;
				}
				else {
					HitMap.Emplace(OtherActor, Damage * Magnification);
				}
			}

			// Hit any object
			if (ImpactParticles) {
				// the same with which in projectile destroyed.
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}

			if (HitSound) {
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-0.5f, 0.5f)
				);
			}
		}

		TArray<ABlasterCharacter*> HitCharacters;
		// apply damage
		for (auto HitPair : HitMap) {
			// Apply Damage
			if (HitPair.Key && InstigatorController) {
				// AuthDamage
				bool bCauseAuthDamage = HasAuthority() && (!bUseServerSideRewind || InstigatorPawn->IsLocallyControlled());
				if (bCauseAuthDamage) {
					UGameplayStatics::ApplyDamage(
						HitPair.Key,
						HitPair.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
				// client ssr
				if (bUseServerSideRewind && !HasAuthority() && InstigatorPawn->IsLocallyControlled()) {
					UE_LOG(LogTemp, Warning, TEXT("FireShotgun() - HitCharacters Added one."));
					HitCharacters.Add(HitPair.Key);
				}
			}
		} // Apply Damage

		// applay ssr damage
		if (!HasAuthority() && InstigatorPawn->IsLocallyControlled() && bUseServerSideRewind) {
			BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(InstigatorPawn) : BlasterOwnerCharacter;
			BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
			if (BlasterOwnerCharacter && BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation()) {
				// whether hit the character or the world static will be check in SSR.
				BlasterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					TraceHitTargets,
					BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime, // location on the client is half RRT before based on server time.
					this
				);
			}
		}

	}	// SocketTransform
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket) return;

	FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	// From muzzle flash socket to hit location from TraceUnderCrosshairs
	FVector TraceStart = SocketTransform.GetLocation();

	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++) {
		FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
		FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		HitTargets.Add(TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH);
	}
}
