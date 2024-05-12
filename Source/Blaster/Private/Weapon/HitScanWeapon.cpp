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
#include "Blaster/Public/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Public/BlasterPlayerController.h"

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
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		UWorld* World = GetWorld();
		if (World) {
			if (FireHit.bBlockingHit) {
				// Only when hit character, the hit is valid to perform SSR
				ABlasterCharacter* OtherActor = Cast<ABlasterCharacter>(FireHit.GetActor());

				// Apply Damage
				// equals to
				bool bCauseAuthDamage = HasAuthority() && (!bUseServerSideRewind || InstigatorPawn->IsLocallyControlled());
				if (bCauseAuthDamage) {
					// When instigator is on the server, we don't really need SSR anyway.
					// when ssr is disabled, damage application is allowed only on server.
					const float Magnification = FireHit.BoneName.ToString() == FString("head") ? HeadShotMagnification : 1.0f;
					UGameplayStatics::ApplyDamage(
						OtherActor,
						Damage * Magnification,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
				// client and ssr
				else if (!HasAuthority() && InstigatorPawn->IsLocallyControlled() && bUseServerSideRewind) {
					BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(InstigatorPawn) : BlasterOwnerCharacter;
					BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
					// since scatter is done locally, hittarget here is simply extended by 1.25 times
					if (BlasterOwnerCharacter && BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation()) {
						BlasterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
							OtherActor,
							Start,
							HitTarget,
							BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime, // location on the client is half RRT before based on server time.
							this
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

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World) {
		//FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25;
		// Apply scatter before fire in CombatCOmponent;
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25;
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
		else {
			OutHit.ImpactPoint = End;
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

		//DrawDebugSphere(GetWorld(), BeamEnd, 20, 12, FColor::Red, true);
	}
}
