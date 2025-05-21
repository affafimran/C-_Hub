#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderCaptureUtils.generated.h"

UCLASS()
class ARINDAL_LEVEL_API URenderCaptureUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Saves a RenderTarget2D as a PNG to disk.
    UFUNCTION(BlueprintCallable, Category = "Capture|RenderTarget")
    static bool SaveRenderTargetToPNG(UTextureRenderTarget2D* RenderTarget, const FString& FileName, const FString& Directory = TEXT(""));
};
