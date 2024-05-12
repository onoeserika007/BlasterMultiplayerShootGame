// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Blaster/Public/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	// why
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (OwnerCharacter && ShotWeapon) {
		if (ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller)) {
			// server calculate damage directly
			// Dynamically spawned actor will always has authority! Even on client, the owner will have autority over the spawned actor.
			// So use the OwnerCharacter->HasAutority() instead.
			bool bCauseAuthDamage = OwnerCharacter->HasAuthority() && (!bUseServerSideRewind || OwnerCharacter->IsLocallyControlled());
			// AuthDamage
			if (bCauseAuthDamage) {
				const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;
				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
			}
			// on client ssr
			else if (!OwnerCharacter->HasAuthority() && bUseServerSideRewind && OwnerCharacter->IsLocallyControlled()) {
				UE_LOG(LogTemp, Display, TEXT("Applying SSR on bullet"));
				if (OwnerCharacter->GetLagCompensation()) {
					ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
					OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
						HitCharacter,
						TraceStart,
						InitialVelocity,
						OwnerController->GetServerTime() - OwnerController->SingleTripTime, // location on the client is half RRT before based on server time.
						ShotWeapon
					);
				}
			}	// Applying Damage

		}	// OwnerController
	}

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	//FPredictProjectilePathParams PredictParams;
	//PredictParams.bTraceWithChannel = true;
	//PredictParams.bTraceWithCollision = true;
	//PredictParams.DrawDebugTime = 5.0f;
	//PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	//PredictParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	//PredictParams.MaxSimTime = 4.0f;
	//PredictParams.ProjectileRadius = 5.0f;
	//PredictParams.SimFrequency = 30.0f;
	//PredictParams.StartLocation = GetActorLocation();
	//PredictParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	//PredictParams.ActorsToIgnore.Add(this);

	//FPredictProjectilePathResult PredictResult;
	//UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);
	if (HasAuthority()) {
		UE_LOG(LogTemp, Warning, TEXT("Auth Bullet Spawn."));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Client Bullet Spawn."));
	}
}
