// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Flag.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	//SetRootComponent(FlagMesh);
	FlagMesh->SetupAttachment(RootComponent);
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetAreaSphere()->SetupAttachment(FlagMesh);

	GetPickupWidget()->SetupAttachment(FlagMesh);
}

void AFlag::BeginPlay()
{
	Super::BeginPlay();

	InitialTransform = GetActorTransform();
}

void AFlag::OnInitial()
{
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorTransform(InitialTransform);
	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlagMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlagMesh->SetRelativeTransform(RelativeTransformFromRootToMesh);
}

void AFlag::OnEquipped()
{
	// at least this can't
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	// Component will be detached from parent after SetSimulatePhysics to true, so it's important to attach back to root.
	FlagMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	FlagMesh->SetRelativeTransform(RelativeTransformFromRootToMesh);
	EnableCustomDepth(false);
}

void AFlag::OnDropped()
{
	//if (HasAuthority()) {
//	//UE_LOG(LogTemp, Warning, TEXT("Set AreaSphere enabled."));
//	
//}
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// once simulate physics is set, WeaponMesh will detach from its parent.
	FlagMesh->SetSimulatePhysics(true);
	FlagMesh->SetEnableGravity(true);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	FlagMesh->MarkRenderStateDirty();	//This will force a refresh
	EnableCustomDepth(true);
}

