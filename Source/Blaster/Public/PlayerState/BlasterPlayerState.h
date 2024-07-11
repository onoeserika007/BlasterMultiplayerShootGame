// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Blaster/Public/BlasterTypes/Team.h"
#include "BlasterPlayerState.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	/** 
	* Replication notifies
	*/
	virtual void OnRep_Score() override;
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

	UFUNCTION(BlueprintGetter)
	FORCEINLINE int32 GetDefeats() const { return Defeats; }

	FORCEINLINE ETeam GetTeam() const { return Team; }
	void SetTeam(ETeam TeamToSet);
	void OnTeamSet();

	UFUNCTION()
	void OnRep_Team();
private:
	// it's important to initialize pointer with nullptr, else Dangling pointer will pass the null check.
	// but, all the pointers and basic types in a class will auto init inplace??????
	// Whether with or without UPROPERTY(), the bug still appears.
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character = nullptr;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller = nullptr;
	
	UPROPERTY(ReplicatedUsing = OnRep_Defeats, Category = PlayerState, BlueprintGetter = GetDefeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;
};
