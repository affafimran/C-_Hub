#include "NSM_Weapon.h"
#include "Engine/ActorChannel.h"
#include "NordicSimMap/Components/Weapon/NSM_WeaponComponent.h"

ANSM_Weapon::ANSM_Weapon()
{
	WeaponComponent = CreateDefaultSubobject<UNSM_WeaponComponent>(TEXT("WeaponComp"));
	AActor::SetReplicateMovement(true);
	bReplicates = true;
}

bool ANSM_Weapon::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	WroteSomething = Channel->ReplicateSubobject(WeaponComponent, *Bunch, *RepFlags);
	return WroteSomething;
}

void ANSM_Weapon::Server_InitializeItem_Implementation()
{
	Multicast_InitializeItem();
}


void ANSM_Weapon::Multicast_InitializeItem_Implementation()
{
	InitializeItem();
	Server_InitializeMesh();
}

void ANSM_Weapon::InitializeItem()
{
	Super::InitializeItem();
	WeaponComponent->Initialize();
}

void ANSM_Weapon::OnRep_ItemState()
{
	Super::OnRep_ItemState();
}
