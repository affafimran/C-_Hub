#pragma once

#include "CoreMinimal.h"
#include "NordicSimMap/Items/Consumable/NSM_ConsumableItem.h"
#include "NSM_AmmunitionItem.generated.h"

/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_AmmunitionItem : public ANSM_ConsumableItem
{
	GENERATED_BODY()

public:
	ANSM_AmmunitionItem();

	virtual void BeginPlay() override;
	virtual void Interact_Implementation(AActor* ActorInteracting) override;
	virtual void OnPickUpOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
};

