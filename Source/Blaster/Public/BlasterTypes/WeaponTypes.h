#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8 {
	EWT_AssaultRifle UMETA(DisplayNmae = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayNmae = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayNmae = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayNmae = "SubmachineGun"),
	EWT_Shotgun UMETA(DisplayNmae = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayNmae = "Sniper Rifle"),
	EWT_GranadeLauncher UMETA(DisplayNmae = "Granade Launcher"),
	EWT_Flag UMETA(DisplayNmae = "Flag"),
	EWT_MAX UMETA(DisplayNmae = "Default Max")
};

#define TRACE_LENGTH 800000.0f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252
