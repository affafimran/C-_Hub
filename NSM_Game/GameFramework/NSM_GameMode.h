#pragma once

#include "CoreMinimal.h"
#include "NSM_GameInstance.h"
#include "GameFramework/GameMode.h"
#include "NSM_GameMode.generated.h"

class AController;
class ANSM_CharacterBase;
/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_GameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ANSM_GameMode();

	virtual void BeginPlay() override;
	
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void PlayerEliminated(ANSM_CharacterBase* EliminatedCharacter, AController* VictimController, AController* AttackerController);

	virtual void RequestRespawn(ACharacter* ElimintedCharacter, AController* EliminatedCharacterController);

	virtual AActor* GetPlayerSpawnArea(UNSM_GameInstance* GameInstance, FGameplayTag TeamTag);
	
	void RegisterExistingPlayers();
	FName CustomSessionName;
private:
	bool bAllExistingPlayersRegistered;
};
