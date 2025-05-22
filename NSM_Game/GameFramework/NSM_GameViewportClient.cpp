#include "NSM_GameViewportClient.h"
#include "SignificanceManager.h"
#include "Kismet/GameplayStatics.h"

void UNSM_GameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (const UWorld* WorldContextObj = GetWorld())
	{
		if (USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(WorldContextObj))
		{			
			TArray<FTransform> TransformArray;
			for (FConstPlayerControllerIterator It = WorldContextObj->GetPlayerControllerIterator(); It; ++It)
			{
				if(const AController* PlayerController = It->Get())
				{
					if (const APawn* PlayerPawn = PlayerController->GetPawn())
					{						
						TransformArray.Add(PlayerPawn->GetTransform());
					}
				}
			}
			
			SignificanceManager->Update(TArrayView<const FTransform>(TransformArray));
		}
	}
}
