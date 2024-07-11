// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);

	//CollisionBox->SetNotifyRigidBodyCollision(true);

	// Projectile as a base class will not be instantialized.
	/*ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;*/
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer) {
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	// bReplicates set after BeginPlay, so this won't work, just enable all collision callback on all machine
	if (bReplicates) {
		if (HasAuthority()) {
			CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
		}
	}
	else {
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
	//CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

#if WITH_EDITOR
void AProjectile::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectile, InitialSpeed)) {
		if (ProjectileMovementComponent) {
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// since the bullet hit and got destoryed to soon, a bullet can't even be replicated to client becofre it was destroyed.
	// So we use multicast instead of Destroyed broadcast.
	if (HasAuthority()) {
		MulticastOnDestroyed();
		bHit = true;
	}
	Destroy();
}

void AProjectile::Destroyed()
{
	if (!bHit) HandleDestroyed();
	Super::Destroyed();
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem) {
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false	// auto destroy
		);
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&ThisClass::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator();
	// hit collision enabled on client for rokect, but damage left for server only.
	if (FiringPawn && HasAuthority()) {
		AController* FiringController = FiringPawn->GetController();
		if (FiringController) {
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,	// World Context obj
				Damage,
				MinimumDamage,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				1.0f,	// DamageFalloff
				UDamageType::StaticClass(),
				TArray<AActor*>(), // IgnoreActors
				this,	// DamageCauser
				FiringController // Instigator Controller
			);
		}
	}
}

void AProjectile::MulticastOnDestroyed_Implementation()
{
	HandleDestroyed();
}

void AProjectile::HandleDestroyed()
{
	UE_LOG(LogTemp, Warning, TEXT("Projectile Destroyed."));
	if (ImpactParticle) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorLocation());
	}

	if (ImpactSound) {
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}
