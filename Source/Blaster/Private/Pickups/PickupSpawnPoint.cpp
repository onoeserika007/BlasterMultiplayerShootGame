// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"
#include "TimerManager.h"

// Sets default values
APickupSpawnPoint::APickupSpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

}

// Called when the game starts or when spawned
void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) {
		StartSpawnTimer((AActor*)nullptr);
	}
}

void APickupSpawnPoint::SpawnPickup()
{
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0) {
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		if (HasAuthority() && SpawnedPickup) {
			SpawnedPickup->OnDestroyed.AddDynamic(this, &ThisClass::StartSpawnTimer);
		}
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority()) {
		SpawnPickup();
	}
}

void APickupSpawnPoint::StartSpawnTimer(AActor* DestroyedActor)
{
	if (DestroyedActor) {
		UE_LOG(LogTemp, Warning, TEXT("Start spawn timer started by pickup."));
	}
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimerMin, SpawnPickupTimerMax);
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&ThisClass::SpawnPickupTimerFinished,
		SpawnTime
	);
}

// Called every frame
void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

