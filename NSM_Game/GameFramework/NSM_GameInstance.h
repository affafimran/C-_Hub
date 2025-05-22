#pragma once

#include "CoreMinimal.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "Engine/GameInstance.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "NSM_GameInstance.generated.h"

class UNSM_LevelData;
class ANSM_RTSPawn;
/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API UNSM_GameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UNSM_GameInstance();

	virtual void Init() override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;
	UFUNCTION()
	ANSM_PlayerController* GetPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Sessions")
	FName CustomSessionName;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category= "Class|Character")
	TSubclassOf<ANSM_CharacterBase> PlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Class|AICharacter")
	TSubclassOf<ANSM_CharacterBase> SquadMemberClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Squad|Info")
	int32 SquadNumbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Squad|Info")
	int32 SquadSize;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Squad|Info")
	bool PlayerAloneInSquad = false;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category= "RTS")
	TSubclassOf<ANSM_RTSPawn> RTSPawnClass;

	UPROPERTY(EditAnywhere)
	TArray<UNSM_FormationDataAsset*> OffensiveFormations;
	
	UPROPERTY(EditAnywhere)
	TArray<UNSM_FormationDataAsset*> StealthFormations;
	
	UPROPERTY(BlueprintReadOnly)
	ANSM_PlayerController* PlayerController;
	
private:
	FDelegateHandle LoginDelegateHandle;
	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

};
