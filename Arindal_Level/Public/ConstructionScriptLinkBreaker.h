#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConstructionScriptLinkBreaker.generated.h"

/**
 * 
 */
UCLASS()
class ARINDAL_LEVEL_API UConstructionScriptLinkBreaker : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

    
    //Breaks all outgoing links from function call nodes with the given function name inside Construction Scripts of the selected actors' Blueprints.

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "ConstructionScriptTools")
    static void BreakFunctionLinksFromConstructionScripts(const TArray<AActor*>& SelectedActors, FName TargetFunctionName);
};