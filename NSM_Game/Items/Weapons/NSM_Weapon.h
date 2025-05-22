#pragma once

#include "CoreMinimal.h"
#include "NordicSimMap/Items/NSM_ItemBase.h"
#include "NSM_Weapon.generated.h"

class UNSM_WeaponComponent;

/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_Weapon : public ANSM_ItemBase
{
	GENERATED_BODY()
	
public:
	ANSM_Weapon();
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(Server, Reliable)
	void Server_InitializeItem();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_InitializeItem();
	
	virtual void InitializeItem() override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UNSM_WeaponComponent* WeaponComponent;
	virtual void OnRep_ItemState() override;
};
