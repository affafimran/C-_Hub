#pragma once

#include "CoreMinimal.h"
#include "NSM_WeaponBase_Refactor.h"
#include "NSM_SniperRifle_Refactor.generated.h"

/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_SniperRifle_Refactor : public ANSM_WeaponBase_Refactor
{
	GENERATED_BODY()
	
public:	
	ANSM_SniperRifle_Refactor();

	void ShowScope();
	void HideScope();

	virtual bool GetShotLocationSocketTransform(FTransform& outTransform) const override;

protected:
	virtual void PlayFireAnimation() override;

private:
	UPROPERTY()
	AActor* ScopeActor = nullptr;
	
	bool bIsOnScope = false;
	
};