// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
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
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
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
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller) {
			Controller->SetHUDDefeats(Defeats);
		}
	}
}
