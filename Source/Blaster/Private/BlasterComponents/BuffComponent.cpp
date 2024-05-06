// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"
#include "Blaster/Public/Charactor/BlasterCharacter.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UBuffComponent::UBuffComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
	HealthUpperBound = FMath::Min(Character->GetMaxHealth(), Character->GetHealth() + HealAmount);
}

bool bReplenishingShield = false;
float ShieldReplenishRate = 0.0f;
float ShieldReplenishAmount = 0.0f;
float ShieldUpperBound = 0.0f;

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	bReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ReplenishTime;
	ShieldReplenishAmount += ShieldAmount;
	ShieldUpperBound = FMath::Min(Character->GetMaxShield(), Character->GetShield() + ShieldAmount);
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || !Character || Character->IsElimmed()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.0f, HealthUpperBound));
	Character->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;
	if (AmountToHeal <= 0.0f) {
		bHealing = false;
		AmountToHeal = 0.0;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bReplenishingShield || !Character || Character->IsElimmed()) return;

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime;
	Character->SetShield(FMath::Clamp(Character->GetShield() + ReplenishThisFrame, 0.0f, ShieldUpperBound));
	Character->UpdateHUDShield();
	ShieldReplenishAmount -= ReplenishThisFrame;
	if (ShieldReplenishAmount <= 0.0f) {
		bReplenishingShield = false;
		ShieldReplenishAmount = 0.0;
	}
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&ThisClass::ResetSpeeds,
		BuffTime
	);

	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&ThisClass::ResetJump,
		BuffTime
	);

	MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
	UE_LOG(LogTemp, Warning, TEXT("Walk speed: %f, crouch speed: %f"), BaseSpeed, CrouchSpeed);
}

void UBuffComponent::SetInitialJumpVeclocity(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::ResetSpeeds()
{
	if (Character == nullptr) return;

	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::ResetJump()
{
	if (Character == nullptr) return;

	MulticastJumpBuff(InitialJumpVelocity);
}


void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (Character && Character->GetCharacterMovement()) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVeclocity)
{
	if (Character && Character->GetCharacterMovement()) {
		Character->GetCharacterMovement()->JumpZVelocity = JumpVeclocity;
	}
}

