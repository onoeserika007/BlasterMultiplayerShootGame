// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Public/BlasterTypes/WeaponTypes.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

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
		FVector End = Start + (HitTarget - Start) * 1.25;
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		UWorld* World = GetWorld();
		if (World) {
			if (FireHit.bBlockingHit) {
				ABlasterCharacter* OtherActor = Cast<ABlasterCharacter>(FireHit.GetActor());

				// Apply Damage
				if (HasAuthority()) {
					if (OtherActor && InstigatorController) {
						UGameplayStatics::ApplyDamage(
							OtherActor,
							Damage,
							InstigatorController,
							this,
							UDamageType::StaticClass()
						);
					}
				}

				if (ImpactParticles) {
					// the same with which in projectile destroyed.
					UGameplayStatics::SpawnEmitterAtLocation(
						World,
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}
				
				if (HitSound) {
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HitSound,
						FireHit.ImpactPoint
					);
				}
			}	// bBlockingHit	

			if (MuzzleFlash) {
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					MuzzleFlash,
					SocketTransform
				);
			}

			if (FireSound) {
				UGameplayStatics::PlaySoundAtLocation(
					this,
					FireSound,
					FireHit.ImpactPoint
				);
			}
		}	// World
	}	// SocketTransform
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
	//DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH, FColor::Green, true);
	return TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH;
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World) {
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25;
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit) {
			BeamEnd = OutHit.ImpactPoint;
		}

		if (BeamParticles) {
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);

			if (Beam) {
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}
