// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

class UProgressBar;
class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> WarmupTime;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> AnnouncementText;
	UPROPERTY(meta = (Bindwidget))
	TObjectPtr<UTextBlock> InfoText;
};
