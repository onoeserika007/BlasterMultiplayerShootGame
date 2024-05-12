// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TeamsGameMode.h"
#include "Blaster/Public/GameState/BlasterGameState.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Public/BlasterTypes/Team.h"

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState) {
		for (auto PState : BGameState->PlayerArray) {
			ABlasterPlayerState* BPState = Cast<ABlasterPlayerState>(PState);
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam) {
				// Red Team
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num()) {
					BGameState->RedTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_RedTeam);
				}
				// Blue Team
				else {
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState) {
		ABlasterPlayerState* BPState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
		if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam) {
			// Red Team
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num()) {
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_RedTeam);
			}
			// Blue Team
			else {
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BPState = Exiting->GetPlayerState<ABlasterPlayerState>();
	if (BGameState && BPState) {
		if (BGameState->RedTeam.Contains(BPState)) {
			BGameState->RedTeam.Remove(BPState);
		}
		if (BGameState->BlueTeam.Contains(BPState)) {
			BGameState->BlueTeam.Remove(BPState);
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	ABlasterPlayerState* AttackerPlayerState = Attacker ? Cast<ABlasterPlayerState>(Attacker->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = Victim ? Cast<ABlasterPlayerState>(Victim->PlayerState) : nullptr;
	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) {
		return BaseDamage;
	}

	if (AttackerPlayerState == VictimPlayerState) {
		return BaseDamage;
	}

	if (VictimPlayerState->GetTeam() == AttackerPlayerState->GetTeam()) {
		return 0.0f;
	}
	return BaseDamage;
}

void ATeamsGameMode::PlayerElimilated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	Super::PlayerElimilated(ElimmedCharacter, VictimController, AttackerController);

	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	if (BGameState && AttackerPlayerState) {
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam) {
			BGameState->BlueTeamScores();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam) {
			BGameState->RedTeamScores();
		}
	}
}
