#include "NSM_SniperRifle_Refactor.h"
#include "Engine/SkeletalMeshSocket.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"

ANSM_SniperRifle_Refactor::ANSM_SniperRifle_Refactor()	
{
}

void ANSM_SniperRifle_Refactor::ShowScope()
{	
	if (bIsOnScope || !ItemData || !ItemData->ScopeClass )
	{
		return;
	}

	if (ANSM_CharacterBase* HolsterCharacter = Cast<ANSM_CharacterBase>(Holster))
	{		
		if (HolsterCharacter->IsLocallyControlled())
		{
			if (UCameraComponent* CameraComponent = HolsterCharacter->GetFollowCamera())
			{				
				ScopeActor = GetWorld()->SpawnActorDeferred<AActor>(
					ItemData->ScopeClass,
					CameraComponent->GetComponentTransform(),
					this,
					HolsterCharacter);

				ScopeActor->FinishSpawning(CameraComponent->GetComponentTransform());
				ScopeActor->AttachToComponent(CameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				ScopeActor->SetActorRelativeLocation(ItemData->ScopeOffset);
				
				if (USkeletalMeshComponent* ScopeSkeletalMesh = ScopeActor->FindComponentByClass<USkeletalMeshComponent>())
				{
					ScopeSkeletalMesh->SetAnimInstanceClass(ItemData->WeaponAnimClass);
				}

				GetWeaponMesh()->SetOwnerNoSee(true);
				bIsOnScope = true;
			}
		}
	}
}

void ANSM_SniperRifle_Refactor::HideScope()
{
	if (IsValid(ScopeActor))
	{		
		ScopeActor->Destroy();
	}
	
	GetWeaponMesh()->SetOwnerNoSee(false);
	bIsOnScope = false;
	ScopeActor = nullptr;
}

bool ANSM_SniperRifle_Refactor::GetShotLocationSocketTransform(FTransform& outTransform) const
{
	if (ScopeActor)
	{
		if (const ANSM_CharacterBase* HolsterCharacter = Cast<ANSM_CharacterBase>(Holster))
		{
			if (HolsterCharacter->IsLocallyControlled())
			{
				if (const UCameraComponent* CameraComponent = HolsterCharacter->GetFollowCamera())
				{
					outTransform = CameraComponent->GetComponentTransform();
					return true;
				}
			}
		}
		
		if (const USkeletalMeshComponent* ScopeSkeletalMesh = ScopeActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (const USkeletalMeshSocket* MuzzleFlashSocket = ScopeSkeletalMesh->GetSocketByName(ItemData->MuzzleSocket))
			{
				outTransform = MuzzleFlashSocket->GetSocketTransform(Cast<USkeletalMeshComponent>(GetMeshComponent()));
				return true;
			}
		}
	}
	
	return Super::GetShotLocationSocketTransform(outTransform);
}

void ANSM_SniperRifle_Refactor::PlayFireAnimation()
{
	
	if(ScopeActor&& IsValid(ItemData->WeaponUsableAnimation))
	{
		if (const USkeletalMeshComponent* ScopeSkeletalMesh = ScopeActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if(UAnimInstance* AnimInstance = ScopeSkeletalMesh->GetAnimInstance())
			{
				AnimInstance->Montage_Play(ItemData->WeaponUsableAnimation);
				return;
			}
		}
	}
	
	Super::PlayFireAnimation();
}
