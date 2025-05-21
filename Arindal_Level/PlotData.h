#pragma once

#include "CoreMinimal.h"
#include "PlotData.generated.h"

USTRUCT(BlueprintType)
struct FPlotData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString Name;

    UPROPERTY(BlueprintReadWrite)
    TArray<FVector2D> Points;
};