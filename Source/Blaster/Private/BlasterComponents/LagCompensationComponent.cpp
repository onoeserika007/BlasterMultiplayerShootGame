// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Blaster.h"
#include "Kismet/GameplayStatics.h"

#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
ULagCompensationComponent::ULagCompensationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	//FFramePackage Package;
	//SaveFramePackage(Package);
	//ShowFramePackage(Package, FColor::Yellow);
}

// Called every frame
void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	SaveHistoryFramePackages();
}

void ULagCompensationComponent::SaveHistoryFramePackages()
{
	if (Character && Character->HasAuthority()) {
		if (FrameHistory.Num() <= 1) {
			FFramePackage ThisFrame;
			SaveFramePackage(ThisFrame);
			FrameHistory.AddHead(ThisFrame);
		}
		else {
			float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
			while (FrameHistory.Num() > 1 && HistoryLength > MaxRecordTime) {
				FrameHistory.RemoveNode(FrameHistory.GetTail());
				HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
			}
			FFramePackage ThisFrame;
			SaveFramePackage(ThisFrame);
			FrameHistory.AddHead(ThisFrame);
		}
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = !Character ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	CacheFramePackage(Character, Package);
}

void ULagCompensationComponent::CacheFramePackage(ABlasterCharacter* HitCharacter, FFramePackage& OutPackage)
{
	if (HitCharacter == nullptr) return;
	OutPackage.Character = HitCharacter;
	OutPackage.Time = GetWorld()->GetTimeSeconds();
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
		if (HitBoxPair.Value) {
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutPackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo) {
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			MaxRecordTime
		);
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0f, 1.0f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;
	InterpFramePackage.Character = YoungerFrame.Character;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo) {
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = OlderFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;

		// Interp = clamp(DeltaTime * InterpSpeed, 0, 1)
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.0f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.0f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

void ULagCompensationComponent::TransformHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
		if (HitBoxPair.Value) {
			const FName& BoxInfoName = HitBoxPair.Key;
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[BoxInfoName].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[BoxInfoName].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[BoxInfoName].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
		if (HitBoxPair.Value) {
			const FName& BoxInfoName = HitBoxPair.Key;
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[BoxInfoName].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[BoxInfoName].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[BoxInfoName].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh()) {
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;

	if (bReturn) return {};

	// Frame History of the HitCharacter
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float YoungestHistoryTime = History.GetHead()->GetValue().Time;

	// Frame package that we check to verify a hit
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	// the older the smaller, the younger the bigger
	if (OldestHistoryTime > HitTime) {
		// too far back - too laggy to do SSR
		return {};
	}

	if (OldestHistoryTime == HitTime) {
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}

	if (YoungestHistoryTime <= HitTime) {
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	// Is Older still younger than HitTime?
	while (Older->GetValue().Time > HitTime) {
		// March back until: OlderTime < HitTime < YoungerTime
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime) {
			Younger = Older;
		}
	}

	// Highly unlikely, but we found our frame to check
	if (Older->GetValue().Time == HitTime) {
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	// Interpolate between Younger and Older
	if (bShouldInterpolate) {
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}

	return FrameToCheck;
}

FServerRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerRewindResult();
	FFramePackage CurrentFrame;
	CacheFramePackage(HitCharacter, CurrentFrame);
	TransformHitBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable collision for the head first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UWorld* World = GetWorld();
	if (World) {
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);

		// hit head
		if (ConfirmHitResult.bBlockingHit) {
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerRewindResult{ true, true };
		}
		// if not hit head, check body
		else {
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
				if (HitBoxPair.Value) {
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			// hit body
			if (ConfirmHitResult.bBlockingHit) {
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerRewindResult{ true, false };
			}
		}
	}
	// no hit
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerRewindResult{ false, false };
}

FServerRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr) return FServerRewindResult();
	FFramePackage CurrentFrame;
	CacheFramePackage(HitCharacter, CurrentFrame);
	TransformHitBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable collision for the head first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FPredictProjectilePathParams PredictParams;
	PredictParams.bTraceWithChannel = true;
	PredictParams.bTraceWithCollision = true;
	PredictParams.DrawDebugTime = 15.0f;
	PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PredictParams.LaunchVelocity = InitialVelocity;
	PredictParams.MaxSimTime = MaxRecordTime;
	PredictParams.ProjectileRadius = 5.0f;
	PredictParams.SimFrequency = 15.0f;
	PredictParams.StartLocation = TraceStart;
	PredictParams.TraceChannel = ECC_HitBox;
	PredictParams.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult PredictResult;
	UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);

	// hit head
	if (PredictResult.HitResult.bBlockingHit) {
		// debug
		if (PredictResult.HitResult.Component.IsValid()) {
			UBoxComponent* Box = Cast<UBoxComponent>(PredictResult.HitResult.Component);
			if (Box) {
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Yellow, true);
			}
		}
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerRewindResult{ true, true };
	}
	// if not hit head, check body
	else {
		for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
			if (HitBoxPair.Value) {
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		
		UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);

		// hit body
		if (PredictResult.HitResult.bBlockingHit) {
			// debug
			if (PredictResult.HitResult.Component.IsValid()) {
				UBoxComponent* Box = Cast<UBoxComponent>(PredictResult.HitResult.Component);
				if (Box) {
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Yellow, true);
				}
			}
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerRewindResult{ true, false };
		}
	}

	// no hit
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerRewindResult{ false, false };
}

FShotgunServerRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& Packages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto& Frame : Packages) {
		if (!Frame.Character) return {};
		UE_LOG(LogTemp, Warning, TEXT("ShotComfirmHitFailed: Not Character ptr in frame."));
	}

	// Result here
	FShotgunServerRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : Packages) {
		FFramePackage CurrentFrame;
		CacheFramePackage(Frame.Character, CurrentFrame);
		TransformHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}

	for (auto& Frame : Packages) {
		// Enable collision for the head first
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	// Perform head Trace
	UWorld* World = GetWorld();
	for (auto& HitLocation : HitLocations) {
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World) {
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter) {
				auto& HitMap = ShotgunResult.HeadShots;
				if (HitMap.Contains(BlasterCharacter)) {
					HitMap[BlasterCharacter]++;
				}
				else {
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}
		}
	}

	for (auto& Frame : Packages) {
		// enable for the rest
		for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes) {
			if (HitBoxPair.Value) {
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		//UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		//HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Perfomr body trace
	for (auto& HitLocation : HitLocations) {
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World) {
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter) {
				auto& HitMap = ShotgunResult.FullShots;
				if (HitMap.Contains(BlasterCharacter)) {
					HitMap[BlasterCharacter]++;
				}
				else {
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}
		}
	}

	for (auto& CurrentFrame : CurrentFrames) {
		ResetHitBoxes(CurrentFrame.Character, CurrentFrame);
		EnableCharacterMeshCollision(CurrentFrame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}

FServerRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	if (FrameToCheck.Time == 0) {
		UE_LOG(LogTemp, Warning, TEXT("FrameToCheck not found."));
		return { false, false };
	}

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	if (FrameToCheck.Time == 0) {
		UE_LOG(LogTemp, Warning, TEXT("ULagCompensationComponent::ProjectileServerSideRewind: FrameToCheck not found."));
		return { false, false };
	}

	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (auto HitCharacter : HitCharacters) {
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
	FServerRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	if (HitCharacter && Confirm.bHitConfirmed && DamageCauser) {
		const float DamageToCause = Confirm.bHeadShot ? DamageCauser->GetDamage() * DamageCauser->GetHeadShotMagnification() : DamageCauser->GetDamage();
		// The Character who owns the LagConpensationComponent is the Damage Instigator.
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToCause,
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime, AWeapon* DamageCauser)
{
	FServerRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	if (!Confirm.bHitConfirmed) {
		UE_LOG(LogTemp, Display, TEXT("ULagCompensationComponent::ProjectileServerScoreRequest_Implementation - Not Confirmed."));
	}
	if (HitCharacter && Confirm.bHitConfirmed && DamageCauser) {
		const float DamageToCause = Confirm.bHeadShot ? DamageCauser->GetDamage() * DamageCauser->GetHeadShotMagnification() : DamageCauser->GetDamage();
		// The Character who owns the LagConpensationComponent is the Damage Instigator.
		UE_LOG(LogTemp, Display, TEXT("ULagCompensationComponent::ProjectileServerScoreRequest_Implementation - Applying Damage."));
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToCause,
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime, AWeapon* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("Calling ShotgunServerScoreRequest_Implementation."));
	UE_LOG(LogTemp, Warning, TEXT("HitCharacters Num: %d"), HitCharacters.Num());
	UE_LOG(LogTemp, Warning, TEXT("HitLocations Num: %d"), HitLocations.Num());
	FShotgunServerRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);
	for (auto& HitCharacter : HitCharacters) {

		if (HitCharacter == nullptr || DamageCauser == nullptr || Character == nullptr) continue;

		float TotalDamage = 0.0f;
		if (Confirm.HeadShots.Contains(HitCharacter)) {
			float ExtraHeadShotDamage = Confirm.HeadShots[HitCharacter] * DamageCauser->GetDamage() * (DamageCauser->GetHeadShotMagnification() - 1.0f);
			TotalDamage += ExtraHeadShotDamage;
		}

		if (Confirm.FullShots.Contains(HitCharacter)) {
			float BaseDamage = Confirm.FullShots[HitCharacter] * DamageCauser->GetDamage();
			TotalDamage += BaseDamage;
		}

		UE_LOG(LogTemp, Warning, TEXT("Applying SSR Damage: %f."), TotalDamage);
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}


