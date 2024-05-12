// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UWidgetAnimation;
/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
	/*
	*	Health
	*/
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> HealthText;

	/*
	*	Shield
	*/
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UProgressBar> ShieldBar;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> ShieldText;

	/**
	*	High Ping
	*/
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UImage> HighPingImage;

	UPROPERTY(meta = (BindwidgetAnim), Transient) // Transient means this is not serialized to disk.
	TObjectPtr<UWidgetAnimation> HighPingAnimation;

	/** 
	*	Scores
	*/

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> ScoreAmount;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> DefeatsAmount;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> RedTeamScore;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> BlueTeamScore;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> TeamScoreSpacerText;

	/**
	*	Weapons
	*/

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> WeaponAmmoAmount;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> CarriedAmmoAmount;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> MatchCountdownText;

	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> GrenadesText;
};
