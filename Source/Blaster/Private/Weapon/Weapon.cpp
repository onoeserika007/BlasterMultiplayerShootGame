// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Blaster/Public/Weapon/Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Public/BlasterTypes/WeaponTypes.h"

static void PrintTraceStack(int Depth = 5)
{
	FString info = "Stack Info:";
	TArray<FProgramCounterSymbolInfo> stacks = FPlatformStackWalk::GetStack(1, Depth);
	for (int i = 0; i < stacks.Num(); i++)
	{
		info += FString("\r\n\t") + FString::Printf(TEXT("%s (%s Line:%d"),
			ANSI_TO_TCHAR(stacks[i].FunctionName), ANSI_TO_TCHAR(stacks[i].Filename), stacks[i].LineNumber);
	}
	if (stacks.Num() > 0)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *info);
	}
}

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = false;
	// have autority only on server
	bReplicates = true;

	// lol lol = ->
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget) {
		PickupWidget->SetVisibility(false);
	}
	
	if (GetLocalRole() == ENetRole::ROLE_Authority || HasAuthority()) {
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget) {
		PickupWidget->SetVisibility(bShowWidget);
	}
}

// called on server and client, and OnRep_WeaponState make sure that the effect is dispatched to client;
// call this on client for redundancy.
// and since it is not a multicast, we should do the same thing on the server as well.
void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState) {
	case EWeaponState::EWS_Equipped:
		// at least this can't
		ShowPickupWidget(false);
		if (HasAuthority()) {
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority()) {
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
	//OnRep_WeaponState();
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState) {
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}

}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr) {
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else {
		// don't let the weapon affect the old owner
		SetHUDWeaponAmmo();
	}
}

void AWeapon::OnRep_Ammo()
{
	SetHUDWeaponAmmo();
}

void AWeapon::SetHUDWeaponAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter->GetLocalRole() == ENetRole::ROLE_AutonomousProxy) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(
				-1,
				15,
				FColor::Yellow,
				FString("SetHUDWeaponAmmo Called on client.")
			);
			PrintScriptCallstack();
		}
	}

	if (BlasterOwnerCharacter) {
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController) {
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SpendRound()
{
	if (HasAuthority()) {
		Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
		SetHUDWeaponAmmo();
	}
}

void AWeapon::Fire()
{
	// Fire events that will be done on all machines.
	if (FireAnimation) {
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass) {
		const USkeletalMeshSocket* AmmoEject = GetWeaponMesh()->GetSocketByName(FName("AmmoEject"));
		if (AmmoEject) {
			FTransform SocketTransform = AmmoEject->GetSocketTransform(GetWeaponMesh());

			UWorld* World = GetWorld();
			if (World) {
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}

	// --ammo
	SpendRound();
}

void AWeapon::Fire(const FVector& HitTarget)
{
	Fire();
}

void AWeapon::Dropped()
{
	// handled by OnRep
	SetWeaponState(EWeaponState::EWS_Dropped);

	// handled automatically by default
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);

	// these will be copied in OnRep
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

void AWeapon::AddAmmo(int32 amount)
{
	Ammo = FMath::Clamp(Ammo + amount, 0, MagCapacity);
	SetHUDWeaponAmmo();
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		// PickupWidget->SetVisibility(true); // this is delegated to the character to do
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		//PickupWidget->SetVisibility(true);
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

