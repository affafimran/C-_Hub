#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "NSM_GameViewportClient.generated.h"

/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API UNSM_GameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;	
};
