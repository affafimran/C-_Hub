#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlotPolygon.h"
#include "RingMeshGenerator.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ARINDAL_LEVEL_API URingMeshGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, CallInEditor)
	static void GenerateAndSpawnRingMesh(const FPlotPolygon& PlotData, float InsetDistance, float ExtrudeHeight);
	
};
