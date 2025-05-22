#include "NSM_Shotgun_Refactor.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"
#include "NordicSimMap/Components/Projectile/NSM_ProjectileMovementComponent.h"
#include "NordicSimMap/Items/NSM_ItemDataAsset.h"
#include "NordicSimMap/Items/Projectiles/NSM_MainProjectile.h"

void ANSM_Shotgun_Refactor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ANSM_Shotgun_Refactor::OnFire(const FVector& TraceHitTarget, bool MissedShot)
{
	//TODO: can be refactored so we don't copy as much code
	if (!HasAmmo())
	{
		return;
	}
	
	if (const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName(GetItemData()->MuzzleSocket))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(Cast<USkeletalMeshComponent>(GetMeshComponent()));	

		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Cast<APawn>(GetOwner());					
			
			UWorld* World = GetWorld();
			if (World)
			{
				for (uint8 i = 0 ; i < NumberOfPellets ; ++i)
				{
					
					// From muzzle flash socket to hit location from trace under crosshairs
					const FRotator& TargetRotation = TraceEndWithScatter(SocketTransform.GetLocation(), TraceHitTarget).Rotation();
					SocketTransform.SetRotation(TargetRotation.Quaternion());
					
					ANSM_MainProjectile* Proj = World->SpawnActorDeferred<ANSM_MainProjectile>(
					ProjectileClass,
					SocketTransform,
					SpawnParams.Owner,
					SpawnParams.Instigator);

					if (Proj)
					{
						Proj->FireCauser = GetHolster();
						Proj->SetLifeSpan(ItemData->AttackRange / Proj->ProjectileMovementComponent->MaxSpeed);
						Proj->FinishSpawning(SocketTransform);
					}	
				}
								
				if (ItemData)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ItemData->CascadeMuzzleFlash.ParticleSystem,
						SocketTransform.GetLocation(),
						SocketTransform.GetRotation().Rotator(),
						ItemData->CascadeMuzzleFlash.Scale);

					
					ANSM_CharacterBase* CharacterHolster = Cast<ANSM_CharacterBase>(GetHolster());
					if(IsValid(ItemData->UsableAnimation) && CharacterHolster)
					{
						CharacterHolster->Server_PlayAnimMontage(ItemData->UsableAnimation);
					}
	
					PlayFireAnimation();
				}
			}

			if (HasAuthority())
			{					
				ConsumeAmmo();
			}
			
			WeaponShotRecoil();
			WeaponShotCamShake();
		}
	}
}
