// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Public/HUD/CharacterOverlay.h"
#include "Blaster/Public/HUD/Announcement.h"

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D	TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);
	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		CrosshairColor
	);
}

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
	//AddCharacterOverlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass) {
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass) {
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X/2.f, ViewportSize.Y/2.f);

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairsCenter) {
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, FVector2D(0.0f), HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsLeft) {
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, FVector2D(-SpreadScaled, 0.0f), HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsRight) {
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, FVector2D(SpreadScaled, 0.0f), HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsTop) {
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, FVector2D(0.0f, -SpreadScaled), HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsBottom) {
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, FVector2D(0.0f, SpreadScaled), HUDPackage.CrosshairsColor);
		}
	}
}
