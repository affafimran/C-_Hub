#pragma once

#include "CoreMinimal.h"
#include "PlotPolygon.generated.h"

USTRUCT(BlueprintType)
struct FPlotPolygon : public FTableRowBase
{
    GENERATED_BODY()

    // Outer polygon in 2D space
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector2D> OuterRing;

    // Location to spawn the final mesh in the world
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector WorldPosition;

    // Name for logging/debugging
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PlotName;
};
