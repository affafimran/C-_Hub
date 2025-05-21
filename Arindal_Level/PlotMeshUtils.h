#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlotData.h"
#include "PlotMeshUtils.generated.h"


USTRUCT(BlueprintType)
struct FPlotMeshEntry : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector WorldLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UMaterialInterface* Material;
};


UCLASS()
class ARINDAL_LEVEL_API UPlotMeshUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Triangulates a simple 2D polygon (no holes), outputs triangle indices */
    UFUNCTION(BlueprintCallable, Category = "Plot|Mesh")
    static void TriangulatePolygon(const TArray<FVector2D>& Vertices, TArray<int32>& Triangles);

    UFUNCTION(BlueprintCallable, Category = "Plot|Mesh")
    static void EarcutTriangulate(const TArray<FVector2D>& Vertices, TArray<int32>& Triangles);

    UFUNCTION(BlueprintCallable, Category = "Plot|Data")
    static bool LoadPlotDataFromFile(const FString& FilePath, TArray<FPlotData>& OutPlots);

    UFUNCTION(BlueprintCallable, Category = "Plot|Export")
    static bool SaveProceduralMeshAsStaticMesh(
        UProceduralMeshComponent* ProcMesh,
        const FString& AssetName,
        const FString& FolderPath = TEXT("/Game/BakedPlots"),
        UMaterialInterface* Material = nullptr
    );

    UFUNCTION(BlueprintCallable, Category = "Plot|Export")
    static bool SavePlotAsActor(
        UProceduralMeshComponent* ProcMesh,
        const FString& ActorName,
        const FString& FolderPath,
        const FVector& WorldLocation,
        UMaterialInterface* Material = nullptr
    );

    UFUNCTION(BlueprintCallable, Category = "Plot|Registry")
    static bool SavePlotToRegistry(
        UProceduralMeshComponent* ProcMesh,
        const FString& AssetName,
        const FString& FolderPath,
        const FVector& WorldLocation,
        UMaterialInterface* Material,
        UDataTable* RegistryTable
    );
};
