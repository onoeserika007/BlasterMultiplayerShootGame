// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
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
private:
	// it's important to initialize pointer with nullptr, else Dangling pointer will pass the null check.
	// but, all the pointers and basic types in a class will auto init inplace??????
	TObjectPtr<ABlasterCharacter> Character;
	TObjectPtr<ABlasterPlayerController> Controller;
	UPROPERTY(ReplicatedUsing = OnRep_Defeats, Category = PlayerState, BlueprintGetter = GetDefeats)
	int32 Defeats;
};
