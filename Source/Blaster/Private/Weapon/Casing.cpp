// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
ACasing::ACasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CasingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CasingMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Block);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECR_Block);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Block);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 10.0f;
}

// Called when the game starts or when spawned
void ACasing::BeginPlay()
{
	Super::BeginPlay();
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(
	//		-1,
	//		3,
	//		FColor::Cyan,
	//		FString("Shell beginplay.")
	//	);
	//}
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(
	//		-1,
	//		3,
	//		FColor::Cyan,
	//		FString("Shell hit.")
	//	);
	//}
	if (ShellSound) {
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
	//Destroy();
	// seconds
	SetLifeSpan(3);
}

// Called every frame
void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

