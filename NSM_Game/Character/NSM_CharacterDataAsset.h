#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NordicSimMap/Core/NSM_NordicData.h"
#include "NSM_CharacterDataAsset.generated.h"

class UNSM_ItemDataAsset;
/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API UNSM_CharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FString ClassName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FText ClassDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FNSM_CharacterData CharacterData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Weapons")
	bool HasEquipment;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Weapons")
	TArray<UNSM_ItemDataAsset*> EquipmentList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Classes")
	FString CharacterClassName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Classes")
	ENSM_CharacterType CharacterClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Character Classes")
	float LineOfSightDistance;
};
