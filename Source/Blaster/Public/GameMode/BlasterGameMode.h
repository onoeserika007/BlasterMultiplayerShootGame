// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState {
	// XXX_API means can be imported from dll.
	extern BLASTER_API const FName Cooldown; // Match duration has been reached. Display winner and begin cool down timer.
};

class ABlasterCharacter;
//class ACharacter;
class ABlasterPlayerController;
class ABlasterPlayerState;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	// GameMode has additional MatchState support than GameModeBase.
public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerElimilated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
public:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.0f;
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.0f;
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.0f;

	float LevelStartingTime = 0.0f;

	bool bTeamsMatch = false;
private:
	float CountdownTime = 0.0f;

public:
	FORCEINLINE float GetCountDownTime() const { return CountdownTime; }
};
