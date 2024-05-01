// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"

namespace MatchState {
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			4,
			15,
			FColor::Yellow,
			FString::Printf(TEXT("Countdown in GameMode: %f"), CountdownTime)
		);
	}

	if (MatchState == MatchState::WaitingToStart) {
		CountdownTime = WarmupTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.0f) {
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress) {
		CountdownTime = WarmupTime  + MatchTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.0f) {
			SetMatchState(MatchState::Cooldown);
		}
	}
	else {
		CountdownTime = CooldownTime + WarmupTime + MatchTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
	}
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartingTime = GetWorld()->GetTimeSeconds();
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			60,
			FColor::Yellow,
			FString::Printf(TEXT("LevelStartingTime in GameMode set : %f at %f"), LevelStartingTime, GetWorld()->GetTimeSeconds())
		);
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) {
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer) {
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerElimilated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState) {
		AttackerPlayerState->AddToScore(1.0f);
	}

	if (VictimPlayerState) {
		VictimPlayerState->AddToDefeats(1.0f);
	}

	if (ElimmedCharacter) {
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter) {
		// here is leaved to be explored more.
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController) {
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		if (PlayerStarts.Num()) {
			int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
			RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
		}
		else {
			if (GEngine) {
				GEngine->AddOnScreenDebugMessage(
					-1,
					15,
					FColor::Red,
					FString("Respawning: No Player Start found!")
				);
			}
		}
	}
}
