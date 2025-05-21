#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UConstructionScriptCleaner.generated.h"

UCLASS()
class ARINDAL_LEVEL_API UConstructionScriptCleaner : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // Clears construction scripts of all Blueprint Actors in the given content folder 
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Editor Scripting")
    static void ClearConstructionScriptsInFolder(const FString& FolderPath);

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Editor Scripting")
    static void ClearConstructionScriptGraphOnly(const FString& FolderPath);
};