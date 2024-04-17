// Fill out your copyright notice in the Description page of Project Settings.


#include "SessionMenu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"

void USessionMenu::MenuSetup(int32 NumPublicConnections, FString MatchType, FString LobbyPath)
{
	// set up params
	NumPublicConnections_ = NumPublicConnections;
	MatchType_ = MatchType;
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World) {
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController) {
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance) {
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsCompleteDelegate.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionCompleteDelegate.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionCompleteDelegate.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool USessionMenu::Initialize()
{ 
	if (!Super::Initialize()) {
		return false;
	}

	if (HostButton) {
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}

	if (JoinButton) {
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void USessionMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void USessionMenu::OnCreateSession(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("Session created successfully! \nSession name: %s"), *SessionName.ToString())
		);

		UWorld* World = GetWorld();
		if (World) {
			World->ServerTravel(PathToLobby);
		}
	}
	else {
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString(TEXT("Failed to create session!"))
		);
		HostButton->SetIsEnabled(true);
	}
}

void USessionMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	for (auto Result : SessionResults) {
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;
		FString MatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);

		// DEBUG
		//if (GEngine) {
		//	GEngine->AddOnScreenDebugMessage(
		//		-1,
		//		15.0f,
		//		FColor::Cyan,
		//		FString::Printf(TEXT("Id: %s, User: %s"), *Id, *User)
		//	);
		//}

		if (MatchType == MatchType_) {
			// DEBUG
			//if (GEngine) {
			//	GEngine->AddOnScreenDebugMessage(
			//		-1,
			//		15.0f,
			//		FColor::Cyan,
			//		FString::Printf(TEXT("Joining Match Type: %s"), *MatchType)
			//	);
			//}

			if (MultiplayerSessionsSubsystem) {
				MultiplayerSessionsSubsystem->JoinSession(Result);
			}
			else {
				if (GEngine) {
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.0f,
						FColor::Red,
						FString::Printf(TEXT("MultiplayerSessionsSubsystem is nullptr!"))
					);
				}
			}

			// found
			return;
		}
	}

	if (SessionResults.Num() == 0 || !bWasSuccessful) {
		JoinButton->SetIsEnabled(true);
	}
}

void USessionMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem) {
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid()) {

			FString Address;
			if (SessionInterface->GetResolvedConnectString(NAME_GameSession, Address)) {
				if (GEngine) {
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.0f,
						FColor::Yellow,
						FString::Printf(TEXT("Connect string: %s"), *Address)
					);
				}

				APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
				if (PlayerController) {
					PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
				}
			}

		}
	}

	if (Result != EOnJoinSessionCompleteResult::Success) {
		JoinButton->SetIsEnabled(true);
	}
}

void USessionMenu::OnDestroySession(bool bWasSuccessful)
{
}

void USessionMenu::OnStartSession(bool bWasSuccessful)
{
}

void USessionMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections_, MatchType_);
	}
}

void USessionMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void USessionMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();
	if (World) {
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController) {
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
