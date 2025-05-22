#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerState.h"
#include "NSM_PlayerStateBase.generated.h"

class ANSM_CharacterBase;
class UNSM_CharacterDataAsset;
/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_PlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
	
	void InitializeSystem();
	
	UFUNCTION()
	void AssignTeam(const int32 PlayerAmount);
		
	void CreateSquad(TArray<ANSM_CharacterBase*> Members, const bool bPlayerSquad = false);
	TArray<ANSM_CharacterBase*> SpawnSquadMembers(const int32 NumberOfMembers, const FVector& LocationToSpawn, UNSM_CharacterDataAsset* NewCharacterDataAsset);
	

	UPROPERTY(EditAnywhere)
	int32 SquadAmount;
	
	UPROPERTY(VisibleAnywhere, Category= "Squad|Info")
	TArray<AActor*> PlayerSquadSpawnAreas;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Squad|Info")
	TArray<AActor*> SpawnAreas;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	FGameplayTag PlayerTeamTag;
private:
	
	void InitializeSpawn();
	void InitializePlayer();
	void InitializeSquads();
};
