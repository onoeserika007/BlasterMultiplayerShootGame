// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "Components/SceneComponent.h"
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
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
	SetReplicateMovement(true);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	//SceneRoot->SetHiddenInGame(true);

	// lol lol = ->
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();	//This will force a refresh
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(WeaponMesh);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(WeaponMesh);
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget) {
		PickupWidget->SetVisibility(false);
	}

	//WeaponMesh->SetSimulatePhysics(true);
	//WeaponMesh->SetEnableGravity(true);
	//WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RelativeTransformFromRootToMesh = WeaponMesh->GetRelativeTransform();
	
	//if (GetLocalRole() == ENetRole::ROLE_Authority || HasAuthority()) {
	//	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	//	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	//	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	//}

	// Overlaps can happens on client, equiping is still handled by server exclusively.
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget) {
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState) {
	case EWeaponState::EWS_Initial:
		OnInitial();
		break;
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}

void AWeapon::OnInitial()
{
}

void AWeapon::OnEquipped()
{
	// at least this can't
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	// Component will be detached from parent after SetSimulatePhysics to true, so it's important to attach back to root.
	WeaponMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetRelativeTransform(RelativeTransformFromRootToMesh);
	if (WeaponType == EWeaponType::EWT_SubmachineGun) {
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	EnableCustomDepth(false);

	// SetHUD
	SetHUDWeaponAmmo();

	// bind pingTooHighDelegate
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && bUseServerSideRewind) {
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority() && !BlasterOwnerController->HighPingDelegate.IsBound()) {
			BlasterOwnerController->HighPingDelegate.AddDynamic(this, &ThisClass::OnPingTooHigh);
		}
	}
}

void AWeapon::OnEquippedSecondary()
{
	// at least this can't
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetRelativeTransform(RelativeTransformFromRootToMesh);
	if (WeaponType == EWeaponType::EWT_SubmachineGun) {
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled()) {
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();	//This will force a refresh
		EnableCustomDepth(true);
	}
	else {
		EnableCustomDepth(false);
	}

	// unbind pingTooHighDelegate
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter) {
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound()) {
			BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
		}
	}
}

void AWeapon::OnDropped()
{
	//if (HasAuthority()) {
	//	//UE_LOG(LogTemp, Warning, TEXT("Set AreaSphere enabled."));
	//	
	//}
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// once simulate physics is set, WeaponMesh will detach from its parent.
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();	//This will force a refresh
	EnableCustomDepth(true);

	// unbind pingTooHighDelegate
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter) {
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound()) {
			BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
		}
	}
}

// called on server and client, and OnRep_WeaponState make sure that the effect is dispatched to client;
// call this on client for redundancy.
// and since it is not a multicast, we should do the same thing on the server as well.
void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
	//OnRep_WeaponState();
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

// this only happens on client of course server is ok
void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr) {
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else {
		// don't let the weapon affect the old owner
		BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() == this) {
			//UE_LOG(LogTemp, Warning, TEXT("Pirmary equipped."));
			SetHUDWeaponAmmo();
		}
		else {
			//UE_LOG(LogTemp, Warning, TEXT("Secondary equipped."));
		}
	}
}

// If our ammo is already at the correct value, this RepNotify isn't going to be called.
void AWeapon::OnRep_Ammo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (WeaponType == EWeaponType::EWT_Shotgun && BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombatComponent() && IsFull()) {
		// Handle shotgun fuul
		BlasterOwnerCharacter->GetCombatComponent()->JumpToShotgunEnd();
	}
	SetHUDWeaponAmmo();
}

void AWeapon::SetHUDWeaponAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	//if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetLocalRole() == ENetRole::ROLE_AutonomousProxy) {
	//	if (GEngine) {
	//		GEngine->AddOnScreenDebugMessage(
	//			-1,
	//			15,
	//			FColor::Yellow,
	//			FString("SetHUDWeaponAmmo Called on client.")
	//		);
	//		PrintScriptCallstack();
	//	}
	//}

	if (BlasterOwnerCharacter) {
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController) {
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh) {
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::SpendRound()
{
	//if (HasAuthority()) {
	//	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	//	SetHUDWeaponAmmo();
	//}
	// Testing for client-side predicting ammo
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDWeaponAmmo();
	if (HasAuthority()) {
		ClientUpdateAmmo(Ammo);
	}
	else {
		// only work on client and local
		AmmoSequence++;
		UE_LOG(LogTemp, Warning, TEXT("AmmoSequence++, %d"), AmmoSequence);
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	AmmoSequence--;
	UE_LOG(LogTemp, Warning, TEXT("AmmoSequence--, %d"), AmmoSequence);
	Ammo -= AmmoSequence;
	SetHUDWeaponAmmo();
}

// TODO: Now there is a lag on client to reload shotgun.
// Try to use client side prediction to figure it out.
// only on server
void AWeapon::AddAmmo(int32 amount)
{
	Ammo = FMath::Clamp(Ammo + amount, 0, MagCapacity);
	SetHUDWeaponAmmo();
	if (HasAuthority()) {
		ClientAddAmmo(amount);
	}
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (WeaponType == EWeaponType::EWT_Shotgun && BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombatComponent() && IsFull()) {
		// Handle shotgun fuul
		BlasterOwnerCharacter->GetCombatComponent()->JumpToShotgunEnd();
	}
	SetHUDWeaponAmmo();
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket) return {};
	FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	// From muzzle flash socket to hit location from TraceUnderCrosshairs
	FVector TraceStart = SocketTransform.GetLocation();

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

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

// At least now it's safr to call locally.
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
	RootComponent->DetachFromComponent(DetachRules);
	SetOwner(nullptr);

	// these will be copied in OnRep
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo >= MagCapacity;
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		// PickupWidget->SetVisibility(true); // this is delegated to the character to do
		UE_LOG(LogTemp, Warning, TEXT("OnSphereOverlap"));
		// can't bear player's own flag
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() == Team && Team != ETeam::ET_NoTeam) return;
		if (BlasterCharacter->IsHoldingTheFlag()) return;
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		//PickupWidget->SetVisibility(true);
		UE_LOG(LogTemp, Warning, TEXT("OnSphereEndOverlap"));
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() == Team && Team != ETeam::ET_NoTeam) return;
		if (BlasterCharacter->IsHoldingTheFlag()) return;
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
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
	//DOREPLIFETIME(AWeapon, Ammo);
}

