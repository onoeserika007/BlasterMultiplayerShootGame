// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/Public/HUD/BlasterHUD.h"
#include "Blaster/Public/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/GameMode/BlasterGameMode.h"
#include "Blaster/Public/HUD/CharacterOverlay.h"
#include "Blaster/Public/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	CheckTimeSync(DeltaTime);
	// poll means lunxun.
	PollInit();


}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.0f;
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HealthBar
		&& BlasterHUD->CharacterOverlay->HealthText;
	if (bHUDValid) {
		UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay;
		const float HealthPercent = Health / MaxHealth;
		Overlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d / %d"), static_cast<int>(Health), FMath::CeilToInt(MaxHealth));
		Overlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else {
		bInitializedCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid) {
		UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay;
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else {
		bInitializedCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->DefeatsAmount;

	if (bHUDValid) {
		UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay;
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else {
		bInitializedCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 WeaponAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay) {
			if (Overlay->WeaponAmmoAmount) {
				FString AmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
				BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
			}
		}
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay) {
			if (Overlay->CarriedAmmoAmount) {
				FString AmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
				BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
			}
		}
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay) {
			if (Overlay->MatchCountdownText) {

				//if (CountdownTime < 0.0f) {
				//	BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
				//	return;
				//}

				int32 Minutes = FMath::FloorToInt(CountdownTime / 60.0f);
				int32 Seconds = CountdownTime - Minutes * 60.0f;
				FString Text = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
				BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(Text));
			}
		}
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->Announcement
		&& BlasterHUD->Announcement->WarmupTime;

	if (bHUDValid) {

		//if (CountdownTime < 0.0f) {
		//	BlasterHUD->Announcement->WarmupTime->SetText(FText());
		//	return;
		//}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.0f);
		int32 Seconds = CountdownTime - Minutes * 60.0f;
		FString Text = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(Text));
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter) {
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) {
		return GetWorld()->GetTimeSeconds();
	}
	else {
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController()) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement) {
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (BlasterHUD->CharacterOverlay) {
			BlasterHUD->CharacterOverlay->RemoveFromParent();
		}
		if (BlasterHUD->Announcement 
			&& BlasterHUD->Announcement->AnnouncementText
			&& BlasterHUD->Announcement->InfoText) {
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncemnetText(" New Match Starts in: ");
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncemnetText));
			BlasterHUD->Announcement->InfoText->SetText(FText());
		}
	}
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr) {
		if (BlasterHUD && BlasterHUD->CharacterOverlay) {
			if (CharacterOverlay = BlasterHUD->CharacterOverlay) {
				// this was done in BlasterCharacter.cpp BeginPlay::UpdateHealthHUD() and PullInit for PlayerState.
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	ServerCheckMatchState();
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.0f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	// lobby level offset? wtf??????? I couldn't ever understand.
	if (HasAuthority()) {
		BlasterGameMode = !BlasterGameMode ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode) {
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountDownTime());
			TimeLeft = BlasterGameMode->GetCountDownTime();
		}
		//if (GEngine) {
		//	GEngine->AddOnScreenDebugMessage(
		//		125,
		//		15,
		//		FColor::Cyan,
		//		FString::Printf(TEXT("Server LevelStartingTime realtime: %f"), LevelStartingTime)
		//	);
		//}
	}

	if (CountdownInt != SecondsLeft) { 
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) {
			SetHUDAnnouncementCountdown(TimeLeft);
			//SetHUDAnnouncementCountdown(BlasterGameMode->GetCountDownTime());
		}
		else if (MatchState == MatchState::InProgress) {
			SetHUDMatchCountdown(TimeLeft);
			//SetHUDMatchCountdown(BlasterGameMode->GetCountDownTime());
		}
	}
	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeOnServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeOnServerReceivedClientRequest + (0.5 * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode) {
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		CooldownTime = GameMode->CooldownTime;
		MatchState = GameMode->GetMatchState();
		// on client and server; client RPC called on server will be executed on all machines
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime);
		//if (GEngine) {
		//	GEngine->AddOnScreenDebugMessage(
		//		-1,
		//		60,
		//		FColor::Yellow,
		//		FString::Printf(TEXT("ServerCheckMatchState_Implementation called at %f"),  GetWorld()->GetTimeSeconds())
		//	);
		//}
	}

	//// on server
	//if (BlasterHUD && MatchState == MatchState::WaitingToStart) {
	//	BlasterHUD->AddAnnouncement();
	//}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	// on server this is set to 0 at first time, maybe because the order of Controller and GameMode's BeginPlay is not certain.
	// they 
	// anyway, get countdown from gameMode directly on server.
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	CooldownTime = Cooldown;
	//if (GEngine) {
	//	GEngine->AddOnScreenDebugMessage(
	//		-1,
	//		60,
	//		FColor::Yellow,
	//		FString::Printf(TEXT("LevelStartingTime in Controller set : %f at %f"), LevelStartingTime, GetWorld()->GetTimeSeconds())
	//	);
	//}

	if (BlasterHUD && MatchState == MatchState::WaitingToStart) {
		BlasterHUD->AddAnnouncement();
	}
	// handle event, since the dispatch from GameMode has been done early away.
	OnMatchStateSet(MatchState);
}
