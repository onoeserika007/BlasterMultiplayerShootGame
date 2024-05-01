// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText) {
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	if (!InPawn) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(
				-1,
				15,
				FColor::Red,
				FString("InPawn missing in UOverheadWidget::ShowPlayerNetRole!")
			);
		}
		return;
	}
	ENetRole LocalRole = InPawn->GetLocalRole();

	auto setRole = [=]()-> auto {
		switch (LocalRole) {
		case ENetRole::ROLE_Authority:
			return FString("Authority");
		case ENetRole::ROLE_AutonomousProxy:
			return FString("Autonomous Proxy");
		case ENetRole::ROLE_SimulatedProxy:
			return FString("Simulated Proxy");
		case ENetRole::ROLE_None:
			return FString("None");
		}
		return FString("Not Set");
	};
	FString Role = setRole();

	//FString Role("No Set");

	//switch (LocalRole) {
	//case ENetRole::ROLE_Authority:
	//	Role =  FString("Authority");
	//case ENetRole::ROLE_AutonomousProxy:
	//	Role =  FString("Autonomous Proxy");
	//case ENetRole::ROLE_SimulatedProxy:
	//	Role =  FString("Simulated Proxy");
	//case ENetRole::ROLE_None:
	//	Role = FString("None");
	//}

	FString LocalRoleString = FString::Printf(TEXT("Local Role: %s"), *Role);
	SetDisplayText(LocalRoleString);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
