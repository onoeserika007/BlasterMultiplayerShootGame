// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

//
//	Declaring out own custom_delegates for the Menu class ti bind callbacks to
//
// DYNAMIC must be compatible to blueprint as well ???? UCLASS is required
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnCreateSessionCompleteDelegate, FName, SessionNamebool, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsCompleteDelegate, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionCompleteDelegate, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionCompleteDelegate, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	//
	//  To handle session functionality. The menus class will call this
	//
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	//
	//	Our own custom delegates for the Menu class to bind callbacks to
	//
	FMultiplayerOnCreateSessionCompleteDelegate MultiplayerOnCreateSessionCompleteDelegate;
	FMultiplayerOnFindSessionsCompleteDelegate MultiplayerOnFindSessionsCompleteDelegate;
	FMultiplayerOnJoinSessionCompleteDelegate MultiplayerOnJoinSessionCompleteDelegate;
	FMultiplayerOnDestroySessionCompleteDelegate MultiplayerOnDestroySessionCompleteDelegate;
	FMultiplayerOnStartSessionCompleteDelegate MultiplayerOnStartSessionCompleteDelegate;

protected:

	//
	//	Internal callbacks for the delegates we'll add to the Online Interface delegate list.
	// These don't need to be called outside this class.
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	bool bCreateSessionOnDestroy{ false };
	int32 LastNumPublicConnections;
	FString LastMatchType;
	//
	//	To add to the Online Session Interface delegate list.
	//	We'll bind our MultiplayerSessionsSubsystem internal callbacks to these.
	//

	//TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;
};
