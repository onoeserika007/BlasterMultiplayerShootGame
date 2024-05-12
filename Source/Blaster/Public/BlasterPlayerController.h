// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class ABlasterHUD;
class UCharacterOverlay;
class ABlasterGameMode;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UReturnToMainMenu;
class ABlasterPlayerState;
class ABlasterGameState;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 WeaponAmmo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);
	void PollInit();
	void CheckTimeSync(float DeltaTime);

	virtual float GetServerTime(); // sync server with world clock
	// Called after this PlayerController's viewport/net connection is associated with this player controller.
	virtual void ReceivedPlayer() override; // sync with the server as soon as possible
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);
	void HandleMatchHasStarted(bool bTeamsMatch = false);
	void HandleCooldown(bool bTeamsMatch = false);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	float SingleTripTime = 0.0f;
	FHighPingDelegate HighPingDelegate;

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void SetHUDTime();

	/**
	*	Sync time between client and server
	*/

	// Requests the current server time, passing in the clients's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeOnServerReceivedClientRequest);

	float ClientServerDelta = 0.0f; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.0f;
	float TimeSyncRunningTime = 0.0f;

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown);

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;

	UFUNCTION()
	void OnRep_ShowTeamScores();

	FString GetInfoText(const TArray<ABlasterPlayerState*>& Players);
	FString GetTeamsInfoText(ABlasterGameState* BlasterGameState);
private:
	/** 
	*	Input
	*/
	UPROPERTY(EditDefaultsOnly, Category = "EnhancedInput")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Menu;

	/** 
	*	Return To main menu
	*/

	UFUNCTION()
	void ToggleViewReturnToMainMenu();

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;
	TObjectPtr<UReturnToMainMenu> ReturnToMainMenu;
	bool bReturnToMainMenuOpen = false;

	TObjectPtr<ABlasterHUD> BlasterHUD;
	TObjectPtr<ABlasterGameMode> BlasterGameMode;
	// should not hard code
	float MatchTime = 0.0f;
	float WarmupTime = 0.0f;
	float LevelStartingTime = 0.0f;
	float CooldownTime = 0.0f;
	uint32 CountdownInt = INT_MAX;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;
	//bool bInitializedCharacterOverlay = false;

	float HUDHealth;
	float HUDShield;
	float HUDMaxHealth;
	float HUDMaxShield;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDWeaponAmmo;
	int32 HUDCarriedAmmo;

	bool bHealthInit = false;
	bool bShiedlInit = false;
	bool bScoreInit = false;
	bool bDefeatsInit = false;
	bool bWeaponAmmoInit = false;
	bool bCarriedAmmoInit = false;

	float HighPingRunningTime = 0.0f;
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.0f;

	float PingAnimationRunningTime = 0.0f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.0f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 420.0f;
};
