#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CSVReaderFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ARINDAL_LEVEL_API UCSVReaderFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Load a file line by line into an array 
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "CSV")
	static bool LoadFileToLines(const FString& FilePath, TArray<FString>& OutLines);
	
};
