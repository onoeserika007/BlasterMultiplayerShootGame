// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> ScoreAmount;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> DefeatsAmount;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> WeaponAmmoAmount;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> CarriedAmmoAmount;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> MatchCountdownText;
};
