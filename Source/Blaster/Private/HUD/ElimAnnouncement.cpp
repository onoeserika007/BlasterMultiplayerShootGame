// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ElimAnnouncement.h"
#include "Components/TextBlock.h"

void UElimAnnouncement::SetElimAnnouncementText(FString AttckName, FString VictimName)
{
	FString ElimAnnouncementText = FString::Printf(TEXT("%s elimmed %s!"), *AttckName, *VictimName);
	if (AnnouncementText) {
		AnnouncementText->SetText(FText::FromString(ElimAnnouncementText));
	}
}
