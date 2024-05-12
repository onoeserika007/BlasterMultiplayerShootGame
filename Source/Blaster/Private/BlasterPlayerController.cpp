// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/Public/HUD/BlasterHUD.h"
#include "Blaster/Public/HUD/CharacterOverlay.h"
#include "Blaster/Public/HUD/ReturnToMainMenu.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/GameMode/BlasterGameMode.h"
#include "Blaster/Public/HUD/CharacterOverlay.h"
#include "Blaster/Public/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Blaster/Public/GameState/BlasterGameState.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/NavigationConfig.h"
#include "Framework/Application/SlateApplication.h"
#include "Blaster/Public/BlasterTypes/AnnouncementText.h"
//#include "Animation/WidgetAnimation.h"

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	CheckTimeSync(DeltaTime);
	// poll means lunxun.
	PollInit();

	CheckPing(DeltaTime);
}


void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.0f;
	}
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			InputSystem->AddMappingContext(InputMappingContext, 1);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent)) {
		if (IA_Menu) {
			EnhancedInputComponent->BindAction(IA_Menu, ETriggerEvent::Started, this, &ThisClass::ToggleViewReturnToMainMenu);
		}
	}
}

void ABlasterPlayerController::ToggleViewReturnToMainMenu()
{
	if (!ReturnToMainMenuWidget) return;

	if (!ReturnToMainMenu) {
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}

	if (ReturnToMainMenu) {
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen) {
			ReturnToMainMenu->MenuSetup();
		}
		else {
			ReturnToMainMenu->MenuTearDown();
		}
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
		bHealthInit = true;
	}
	HUDHealth = Health;
	HUDMaxHealth = MaxHealth;
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ShieldBar
		&& BlasterHUD->CharacterOverlay->ShieldText;
	if (bHUDValid) {
		UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay;
		const float Percent = Shield / MaxShield;
		Overlay->ShieldBar->SetPercent(Percent);
		FString Text = FString::Printf(TEXT("%d / %d"), static_cast<int>(Shield), FMath::CeilToInt(MaxShield));
		Overlay->ShieldText->SetText(FText::FromString(Text));
		bShiedlInit = true;
	}
	HUDShield = Shield;
	HUDMaxShield = MaxShield;
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
		bScoreInit = true;
	}
	HUDScore = Score;
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
		bDefeatsInit = true;
	}
	HUDDefeats = Defeats;
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 WeaponAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay) {
			if (Overlay->WeaponAmmoAmount) {
				FString AmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
				BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
				bWeaponAmmoInit = true;
			}
		}
	}
	HUDWeaponAmmo = WeaponAmmo;
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (HasAuthority()) {
		if (!BlasterHUD) {
			UE_LOG(LogTemp, Warning, TEXT("Autority that is not locally controlled has controller on server but without a HUD!"));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Autority has controller on server with a HUD only when is locally controlled!"));
		}
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid) {
		UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay;
		FString AmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
		bCarriedAmmoInit = true;
	}
	HUDCarriedAmmo = CarriedAmmo;
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay) {
			if (Overlay->MatchCountdownText) {

				if (CountdownTime < 0.0f) {
					BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
					return;
				}

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

		if (CountdownTime < 0.0f) {
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.0f);
		int32 Seconds = CountdownTime - Minutes * 60.0f;
		FString Text = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(Text));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->GrenadesText;
	if (bHUDValid) {
		UCharacterOverlay* Overlay = BlasterHUD->CharacterOverlay;
		FString Text = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(Text));
	}
	//else {
	//	bInitializedCharacterOverlay = true;
	//	HUDGrenades = Grenades;
	//}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter) {
		//SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		//SetHUDHealth()
		BlasterCharacter->SpawnDefaultWeapon();
		BlasterCharacter->UpdateHUDAmmo();
		BlasterCharacter->UpdateHUDHealth();
		BlasterCharacter->UpdateHUDShield();
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

// handle HUDs
void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
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

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores) {
		InitTeamScores();
	}
	else {
		HideTeamScores();
	}
}


void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		if (BlasterHUD->CharacterOverlay == nullptr) {
			BlasterHUD->AddCharacterOverlay();
		}

		if (BlasterHUD->Announcement) {
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (HasAuthority()) {
			bShowTeamScores = bTeamsMatch;
			if (bTeamsMatch) {
				InitTeamScores();
			}
			else {
				HideTeamScores();
			}
		}
	}
}

void ABlasterPlayerController::HandleCooldown(bool bTeamsMatch)
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
			FString AnnouncemnetText(Announcement::NewMatchStartsIn);
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncemnetText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* LocalPlayerState = GetPlayerState<ABlasterPlayerState>();
			FString InfoTextString;
			if (BlasterGameState && LocalPlayerState) {
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				InfoTextString = bShowTeamScores? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers);
			}

			BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombatComponent()) {
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}

FString ABlasterPlayerController::GetInfoText(const TArray<ABlasterPlayerState*>& Players)
{
	FString InfoTextString;
	ABlasterPlayerState* LocalPlayerState = GetPlayerState<ABlasterPlayerState>();
	if (!LocalPlayerState) return {};
	if (Players.Num() == 0) {
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == LocalPlayerState) {
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1) {
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
	}
	else if (Players.Num() > 1) {
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::PlayersAreTiedToWin);
		for (auto TiedPlayer : Players) {
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}
	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText(ABlasterGameState* BlasterGameState)
{
	if (!BlasterGameState) return {};
	FString InfoTextString;
	 
	int32 RedTeamScore = BlasterGameState->RedTeamScore;
	int32 BlueTeamScore = BlasterGameState->BlueTeamScore;
	if (RedTeamScore == 0 && BlueTeamScore == 0) {
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore) {
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(FString::Printf(TEXT("%s\n"), *Announcement::BlueTeam));
		InfoTextString.Append(FString::Printf(TEXT("%s\n"), *Announcement::RedTeam));
	}
	else if (RedTeamScore > BlueTeamScore) {
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::RedTeamWins);
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore) {
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::BlueTeamWins);
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, RedTeamScore));
	}
	return FString();
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HighPingAnimation
		&& BlasterHUD->CharacterOverlay->HighPingImage;
	if (bHUDValid) {
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.0);
		BlasterHUD->CharacterOverlay->PlayAnimation(
			BlasterHUD->CharacterOverlay->HighPingAnimation,
			0.0f,
			5
		);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HighPingAnimation
		&& BlasterHUD->CharacterOverlay->HighPingImage;
	if (bHUDValid) {
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.0f);
		if (BlasterHUD->CharacterOverlay->IsPlayingAnimation()) {
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency) {
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState) {
			// Ping is Compressed
			UE_LOG(LogTemp, Display, TEXT("PlayerState->GetPing() * 4: %d"), PlayerState->GetCompressedPing());
			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold) {
				HighPingWarning();
				PingAnimationRunningTime = 0;
				ServerReportPingStatus(true);
			}
			else {
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.0f;
	}

	bool bHighPingAnimPlaying = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingAnimation &&
		BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimPlaying) {
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime >= HighPingDuration) {
			StopHighPingWarning();
			PingAnimationRunningTime = 0;
		}
	}
}

void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::PollInit()
{
	if (!bHealthInit || !bShiedlInit || !bScoreInit || !bDefeatsInit || !bWeaponAmmoInit || !bCarriedAmmoInit) {
		if (BlasterHUD && BlasterHUD->CharacterOverlay) {
			UE_LOG(LogTemp, Warning, TEXT("Controller Polling."));
			//if (!bHealthInit || !bShiedlInit || !bScoreInit || !bDefeatsInit) return;
			if (CharacterOverlay = BlasterHUD->CharacterOverlay) {
				// this was done in BlasterCharacter.cpp BeginPlay::UpdateHealthHUD() and PullInit for PlayerState.
				if (!bHealthInit) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (!bShiedlInit) SetHUDShield(HUDShield, HUDMaxShield);
				if (!bScoreInit) SetHUDScore(HUDScore);
				if (!bDefeatsInit) SetHUDDefeats(HUDDefeats);
				if (!bWeaponAmmoInit) SetHUDWeaponAmmo(HUDWeaponAmmo);
				if (!bCarriedAmmoInit) SetHUDCarriedAmmo(HUDCarriedAmmo);
				//SetHUDGrenades(HUDGrenades);
				ABlasterCharacter* Actor = Cast<ABlasterCharacter>(GetPawn());
				if (Actor && Actor->GetCombatComponent()) {
					SetHUDGrenades(Actor->GetCombatComponent()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamScores);
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->RedTeamScore
		&& BlasterHUD->CharacterOverlay->BlueTeamScore
		&& BlasterHUD->CharacterOverlay->TeamScoreSpacerText;
	if (bHUDValid) {
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->TeamScoreSpacerText->SetText(FText());
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->RedTeamScore
		&& BlasterHUD->CharacterOverlay->BlueTeamScore
		&& BlasterHUD->CharacterOverlay->TeamScoreSpacerText;
	if (bHUDValid) {
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString("0"));
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(":"));
		BlasterHUD->CharacterOverlay->TeamScoreSpacerText->SetText(FText::FromString("0"));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->RedTeamScore;
	if (bHUDValid) {
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->BlueTeamScore;
	if (bHUDValid) {
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	ServerCheckMatchState();

	//if (IsLocalPlayerController())
	//{
	//	TSharedRef<FNavigationConfig> Navigation = MakeShared<FNavigationConfig>();
	//	Navigation->bKeyNavigation = false;
	//	Navigation->bTabNavigation = false;
	//	Navigation->bAnalogNavigation = false;
	//	FSlateApplication::Get().SetNavigationConfig(Navigation);
	//}
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

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self) {
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD) {
			if (Attacker == Self) {
				if (Victim != Self) {
					BlasterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				}
				else {
					BlasterHUD->AddElimAnnouncement("You", "yourself");
				}
			}
			else {
				if (Victim == Self) {
					BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
				}
				else {
					BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
				}
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeOnServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5 * RoundTripTime;
	float CurrentServerTime = TimeOnServerReceivedClientRequest + SingleTripTime;
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
