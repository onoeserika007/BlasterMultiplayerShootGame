// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	// maybe the Character here is detroyed but not set to nullptr yet, if we access to it may access a dangling pointer.
	// now we know here character has an Invalid Label but with a valid address
	if (Character) {
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller) {
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

// onServer
void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller) {
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;
	switch (TeamToSet)
	{
	case ETeam::ET_RedTeam:
		UE_LOG(LogTemp, Display, TEXT("Team Red Set."));
		break;
	case ETeam::ET_BlueTeam:
		UE_LOG(LogTemp, Display, TEXT("Team Blue Set."));
		break;
	case ETeam::ET_NoTeam:
		UE_LOG(LogTemp, Display, TEXT("Team None Set."));
		break;
	case ETeam::ET_MAX:
		break;
	default:
		break;
	}
	OnTeamSet();
}

void ABlasterPlayerState::OnTeamSet()
{
	ABlasterCharacter* BCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BCharacter) {
		BCharacter->SetTeamColor(Team);
	}
}

void ABlasterPlayerState::OnRep_Team()
{
	OnTeamSet();
}
