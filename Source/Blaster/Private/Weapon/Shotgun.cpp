// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	// Grandparent Class
	AWeapon::Fire(HitTarget);

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

		TMap<ABlasterCharacter*, uint32> HitMap;
		for (uint32 i = 0; i < NumberOfPellets; i++) {
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* OtherActor = Cast<ABlasterCharacter>(FireHit.GetActor());

			// Hit Character
			if (HasAuthority() && OtherActor && InstigatorController) {
				if (HitMap.Contains(OtherActor)) {
					HitMap[OtherActor]++;
				}
				else {
					HitMap.Emplace(OtherActor, 1);
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

		for (auto HitPair : HitMap) {
			// Apply Damage
			if (HasAuthority() && HitPair.Key && InstigatorController) {
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}

	}	// SocketTransform
}
