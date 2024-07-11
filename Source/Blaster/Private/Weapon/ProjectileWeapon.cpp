// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Public/Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();

	if (MuzzleFlashSocket && World) {
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// From muzzle flash socket to hit location from TraceUnderCrosshairs
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		// bUseServerSideRewind here is something different from "no SSR" in the comment.
		// just make it aligned with the weapon.
		// whether and how to apply damage depends on multiple bools.
		
		if (bUseServerSideRewind && ServerSideRewindProjectileClass && ProjectileClass ) {
			// server
			if (InstigatorPawn->HasAuthority()) {
				// server, host - use replicated projectile - take damage
				if (InstigatorPawn->IsLocallyControlled()) {
					UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - SSR Auth Local Firing"));
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams); // rep true
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->Damage = Damage; // set damage of projectile as weapon's
					SpawnedProjectile->HeadShotDamage = Damage * HeadShotMagnification;
				}
				// server, not locally controlled - spawn non-replicated projectile, no SSR - take no damage
				else {
					UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - SSR Auth NoLocal Firing"));
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams); // rep false
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
			}
			// client, using SSR
			else {
				// client, locally controlled - spawn non-replicated projectile, use SSR - take damage
				if (InstigatorPawn->IsLocallyControlled()) {
					UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - SSR Client Local Firing"));
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->Damage = Damage; // set damage of projectile as weapon's
					SpawnedProjectile->HeadShotDamage = Damage * HeadShotMagnification;
					/*When a client spawns an actor it will have authority over this actor. 
					Therefore you should spawn any network relevant actor on the server and then replicate them over to the clients. 
					Then also when a client wants to use a perk you should replicate that over to the server and execute it there.*/
					//if (SpawnedProjectile->GetOwner()->HasAuthority()) {
					//	UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - SSR Client Local Firing - It's weird that owner has a authority!"));
					//}
					//if (SpawnedProjectile->IsReplicated()) {
					//	UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - SSR Client Local Firing - It's weird that the projectile is replicated!"));
					//}
				}
				// client, not locally controlled - spawn non-replicated projectile, no SSR - take no damage
				else {
					UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - SSR Client NoLocal Firing"));
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
			}
		}
		// weapon not using SSR - take damage
		else {
			if (InstigatorPawn->HasAuthority() && ProjectileClass ) {
				UE_LOG(LogTemp, Display, TEXT("AProjectileWeapon::Fire - NoSSR Auth Firing"));
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams); // rep true
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage; // set damage of projectile as weapon's
				SpawnedProjectile->HeadShotDamage = Damage * HeadShotMagnification;
			}
		}
		
		if (SpawnedProjectile) {
			SpawnedProjectile->ShotWeapon = this;
		}
	} // MuzzleFlashSocket && World && ProjectileClass && ServerSideRewindProjectileClass

}
