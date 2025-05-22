#include "NSM_ItemBase.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"
#include "NordicSimMap/Components/Equipment/NSM_EquipmentComponent.h"
#include "NordicSimMap/Core/NSM_GeneralStatics.h"
#include "NordicSimMap/Core/NSM_NordicData.h"
#include "NordicSimMap/GAS/NSM_AbilitySystemComponent.h"
#include "NordicSimMap/GAS/NSM_BaseGameplayAbility.h"
#include "Weapons/Refactor/NSM_WeaponBase_Refactor.h"

// Sets default values
ANSM_ItemBase::ANSM_ItemBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystemComponent = CreateDefaultSubobject<UNSM_AbilitySystemComponent>(TEXT("AbilitySystemComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	
	AActor::SetReplicateMovement(true);
	bIsInCooldown = false;
	bReplicates = true;
	CooldownRemaining = 0.0f;
}

void ANSM_ItemBase::BeginPlay()
{
	Super::BeginPlay();
	if(HasAuthority())
	{
		InitializeItem();
	}
}

void ANSM_ItemBase::UseLocal()
{
}

void ANSM_ItemBase::Server_PlayAnimMontage_Implementation(UAnimMontage* Montage)
{
	Multicast_PlayAnimMontage(Montage);
}

void ANSM_ItemBase::Multicast_SetHolster_Implementation(AActor* NewHolster)
{
	Holster = NewHolster;
}

void ANSM_ItemBase::Server_SetHolster_Implementation(AActor* NewHolster)
{
	Multicast_SetHolster(NewHolster);
}

bool ANSM_ItemBase::Server_SetHolster_Validate(AActor* NewHolster)
{
	return true;
}

void ANSM_ItemBase::OnRep_ItemState()
{
	switch (ItemState) {
	case ENSM_ItemState::Initial:
		break;
	case ENSM_ItemState::Equipment:
		SkeletalMeshComponent->SetSimulatePhysics(false);
		SkeletalMeshComponent->SetEnableGravity(false);
		SetActorHiddenInGame(!ItemData->bShowInEquipment);
		PickUpCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case ENSM_ItemState::Equipped:
		SetActorHiddenInGame(false);
		SkeletalMeshComponent->SetEnableGravity(false);
		SkeletalMeshComponent->SetSimulatePhysics(false);
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickUpCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case ENSM_ItemState::Dropped:
		if (IsValid(SkeletalMeshComponent))
		{
			SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			SkeletalMeshComponent->SetSimulatePhysics(true);
			SkeletalMeshComponent->SetEnableGravity(true);
		}
		if(HasAuthority())
		{
			if(IsValid(PickUpCollision))
			{
				if (PickUpCollision->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
				{
					PickUpCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				}
			}
		}
		break;
	case ENSM_ItemState::MAX: break;
	default: ;
	}
}
void ANSM_ItemBase::Multicast_UseItem_Implementation(FGameplayTag AbilityTag, AActor* SourceActor)
{
	if (ANSM_CharacterBase* SourceCharacter = Cast<ANSM_CharacterBase>(SourceActor))
	{
		if (SourceCharacter->IsLocallyControlled() || !SourceCharacter->GetController())
		{
			UseLocal();
		}
	}
	else
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(AbilityTag);
		if (IsValid(AbilitySystemComponent) && HasAuthority())
		{
			AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
		}		
	}
}

void ANSM_ItemBase::Server_PickUp_Implementation(const bool bIsOverlapping, AActor* PickedUpBy)
{	
	if (!IsValid(PickedUpBy) || !IsValid(ItemData))
	{
		return;
	}

	UNSM_EquipmentComponent* EquipmentComponent = PickedUpBy->FindComponentByClass<UNSM_EquipmentComponent>();
	if (!IsValid(EquipmentComponent))
	{
		return;
	}
	
	EquipmentComponent->AddItem(this);
	Server_SetHolster(PickedUpBy);
}

int32 ANSM_ItemBase::GetMaxQuantity() const
{
	if(ItemData)
	{
		return ItemData->MaxQuantity;
	}
	return 1.f;
}


UNSM_ItemDataAsset* ANSM_ItemBase::GetItemData() const
{
	return ItemData;
}

void ANSM_ItemBase::SetItemState_Implementation(ENSM_ItemState NewItemState)
{
	ItemState = NewItemState;
	OnRep_ItemState();
}

void ANSM_ItemBase::StartCooldown()
{
	if(ItemData)
	{
		if (ItemData->UseRate > 0.0f)
		{
			bIsInCooldown = true;
			CooldownRemaining = ItemData->UseRate;
		}
	}
}

void ANSM_ItemBase::EndCooldown()
{
	bIsInCooldown = false;
	CooldownRemaining = 0.0f;
}

void ANSM_ItemBase::OnItemDataChanged()
{
	Super::OnItemDataChanged();
	InitializeItem();
}

void ANSM_ItemBase::Interact_Implementation(AActor* ActorInteracting)
{
	if (ItemData->ItemType == ENSM_ItemType::Weapon)
	{
		INSM_InteractableActor::Interact_Implementation(ActorInteracting);
		Server_PickUp(false, ActorInteracting);
	}
}


void ANSM_ItemBase::Multicast_PlayAnimMontage_Implementation(UAnimMontage* Montage)
{
	UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();
	if(!IsValid(AnimInstance))
	{
		return;
	}
	AnimInstance->Montage_Play(Montage);
}

void ANSM_ItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSM_ItemBase, ItemState);
}

void ANSM_ItemBase::Server_UseItem_Implementation(FGameplayTag AbilityTag, AActor* SourceActor)
{
	Multicast_UseItem(AbilityTag, SourceActor);
}

bool ANSM_ItemBase::Server_UseItem_Validate(FGameplayTag AbilityTag, AActor* SourceActor)
{
	return true;
}

void ANSM_ItemBase::AddStartupGameplayAbilities()
{
	if(!IsValid(AbilitySystemComponent))
	{
		return;
	}
		AbilitySystemComponent->ClearAllAbilities();
		for (const TSubclassOf<UNSM_BaseGameplayAbility>& StartupAbility : ItemData->ItemAbilities)
		{
			if (IsValid(StartupAbility))
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(
					StartupAbility,
					1,
					static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputId),
					this));
			}
		}
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(
				ItemData->DamageEffect, 1, EffectContext);

			if(NewHandle.IsValid())
			{
				FActiveGameplayEffectHandle ActiveGameplayEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent);
			}
		bAbilitiesInitialized = true;
}

// Called every frame
void ANSM_ItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ANSM_ItemBase::InitializeItem()
{
	//AddStartupGameplayAbilities();

	if (HasAuthority())
	{
		PickUpCollision->OnComponentBeginOverlap.Clear();
		PickUpCollision->SetCollisionEnabled(
			ItemState == ENSM_ItemState::Equipment || ItemState == ENSM_ItemState::Equipped
			? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		PickUpCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		PickUpCollision->OnComponentBeginOverlap.AddDynamic(this, &ANSM_ItemBase::OnPickUpOverlapBegin);
	}
	
	if(!IsValid(SkeletalMeshComponent))
	{
		return;
	}
	if(!IsValid(ItemData) || !IsValid(ItemData->WeaponAnimClass))
	{
		return;
	}
	SkeletalMeshComponent->SetAnimInstanceClass(ItemData->WeaponAnimClass);
	Server_InitializeMesh();
}

void ANSM_ItemBase::OnPickUpOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Server_PickUp(true, OtherActor);
}

void ANSM_ItemBase::UpdateCooldown(float DeltaTime)
{
	if (bIsInCooldown)
	{
		CooldownRemaining -= DeltaTime;

		if (CooldownRemaining <= 0.0f)
		{
			EndCooldown();
		}
	}
}


	