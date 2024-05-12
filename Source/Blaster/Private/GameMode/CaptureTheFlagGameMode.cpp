// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CaptureTheFlagGameMode.h"
#include "Blaster/Public/Weapon/Flag.h"
#include "Blaster/Public/CaptureTheFlag/FlagZone.h"
#include "Blaster/Public/GameState/BlasterGameState.h"
#include "Kismet/GameplayStatics.h"

void ACaptureTheFlagGameMode::PlayerElimilated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerElimilated(ElimmedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team;
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState && bValidCapture) {
		UE_LOG(LogTemp, Display, TEXT("Flag Captured Scoring."));
		if (Zone->Team == ETeam::ET_BlueTeam) {
			BGameState->BlueTeamScores();
		}
		if (Zone->Team == ETeam::ET_RedTeam) {
			BGameState->RedTeamScores();
		}
	}
}
