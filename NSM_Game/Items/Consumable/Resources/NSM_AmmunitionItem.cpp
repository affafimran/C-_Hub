#include "NSM_AmmunitionItem.h"
#include "Components/SphereComponent.h"
#include "NordicSimMap/Components/Equipment/NSM_EquipmentComponent.h"
#include "NordicSimMap/Components/Weapon/NSM_WeaponComponent.h"

ANSM_AmmunitionItem::ANSM_AmmunitionItem()
{
	StaticMeshComponent->OnComponentBeginOverlap.AddDynamic(this, &ANSM_AmmunitionItem::OnPickUpOverlapBegin);
}

void ANSM_AmmunitionItem::BeginPlay()
{
	Super::BeginPlay();
	
}

void ANSM_AmmunitionItem::Interact_Implementation(AActor* ActorInteracting)
{
	if(!IsValid(ItemData))
	{
		return;
	}
	
	if(!IsValid(ActorInteracting))
	{
		return;
	}

	UNSM_EquipmentComponent* EquipmentComponent = ActorInteracting->FindComponentByClass<UNSM_EquipmentComponent>();
	if(!IsValid(EquipmentComponent))
	{
		return;
	}

	const ANSM_ItemBase* ItemSelected = EquipmentComponent->FindItemByType(ItemData->AmmoType);
	if(!IsValid(ItemSelected))
	{
		return;
	}
	
	UNSM_WeaponComponent* WeaponComponent = ItemSelected->FindComponentByClass<UNSM_WeaponComponent>();
	if(!IsValid(WeaponComponent))
	{
		return;
	}

	int NewAmmo = WeaponComponent->GetCurrentAmmo() + FMath::RandRange(ItemData->Quantity, ItemData->MaxQuantity);
	
	if(ItemSelected->ItemData->MaxBulletsCapacity < NewAmmo)
	{
		NewAmmo = ItemSelected->ItemData->MaxBulletsCapacity;
	}
	WeaponComponent->SetCurrentAmmo(NewAmmo);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickUpCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);
	SetLifeSpan(3.f);
}

void ANSM_AmmunitionItem::OnPickUpOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnPickUpOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	Interact_Implementation(OtherActor);
}