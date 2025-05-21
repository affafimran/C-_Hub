// Fill out your copyright notice in the Description page of Project Settings.


#include "RingMeshGenerator.h"
#include "PolygonOffsetHelper.h"
#include "PolygonTriangulator.h"
#include "ProceduralMeshComponent.h"
#include "PlotPolygon.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"


void URingMeshGenerator::GenerateAndSpawnRingMesh(const FPlotPolygon& PlotData, float InsetDistance, float ExtrudeHeight)
{
    const TArray<FVector2D>& Outer = PlotData.OuterRing;
    TArray<FVector2D> Inner = PolygonOffsetHelper::OffsetPolygonInward(Outer, -InsetDistance);

    if (Outer.Num() < 3 || Inner.Num() < 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid polygon data for %s"), *PlotData.PlotName);
        return;
    }

    TArray<int32> Triangles = UPolygonTriangulator::TriangulateWithEarcut(Outer, Inner);
    TArray<FVector> Vertices;

    // Convert 2D to 3D points for top surface
    for (const FVector2D& pt : Outer)
        Vertices.Add(FVector(pt.X, pt.Y, 0.f));

    for (const FVector2D& pt : Inner)
        Vertices.Add(FVector(pt.X, pt.Y, 0.f));

    const int32 VertexCount = Vertices.Num();

    // Extrude bottom side (optional)
    for (int32 i = 0; i < VertexCount; i++)
    {
        FVector Bottom = Vertices[i];
        Bottom.Z -= ExtrudeHeight;
        Vertices.Add(Bottom);
    }

    // Duplicate triangles for bottom face (reversed)
    TArray<int32> AllTriangles = Triangles;

    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        AllTriangles.Add(Triangles[i + 2] + VertexCount);
        AllTriangles.Add(Triangles[i + 1] + VertexCount);
        AllTriangles.Add(Triangles[i] + VertexCount);
    }

    // TODO: Add extrusion side walls here (optional for later)

    // Spawn new Actor with Procedural Mesh
    UWorld* World = GEditor->GetEditorWorldContext().World();
    AActor* NewActor = World->SpawnActor<AActor>(AActor::StaticClass(), PlotData.WorldPosition, FRotator::ZeroRotator);

    UProceduralMeshComponent* ProcMesh = NewObject<UProceduralMeshComponent>(NewActor);
    ProcMesh->RegisterComponent();
    ProcMesh->AttachToComponent(NewActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

    ProcMesh->CreateMeshSection_LinearColor(0, Vertices, AllTriangles, {}, {}, {}, {}, true);
    ProcMesh->SetMaterial(0, nullptr); // Set default or custom material here

    UE_LOG(LogTemp, Log, TEXT("Spawned ring mesh for %s with %d vertices."), *PlotData.PlotName, Vertices.Num());
}