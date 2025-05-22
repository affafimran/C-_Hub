#pragma once

#include "CoreMinimal.h"
#include "NSM_WeaponBase_Refactor.h"
#include "NSM_Shotgun_Refactor.generated.h"

/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_Shotgun_Refactor : public ANSM_WeaponBase_Refactor
{
	GENERATED_BODY()

public:	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void OnFire(const FVector& TraceHitTarget, bool MissedShot = false) override;
	
	

	UPROPERTY(EditDefaultsOnly)
	uint8 NumberOfPellets = 6;
};
