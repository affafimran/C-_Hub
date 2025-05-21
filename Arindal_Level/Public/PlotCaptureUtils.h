#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlotCaptureUtils.generated.h"

UCLASS()
class ARINDAL_LEVEL_API UPlotCaptureUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Plot Capture")
    static void CaptureAllPlotsFromBlueprints();

private:

    static class ASceneCapture2D* FindCaptureCamera(UWorld* World);
    static void CaptureSinglePlot(AActor* PlotActor, ASceneCapture2D* CaptureCamera, const FString& SaveDir);
    static FVector GetSavedWorldPosition(AActor* Actor);
};

