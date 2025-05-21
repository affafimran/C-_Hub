#include "PlotCaptureUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"

void UPlotCaptureUtils::CaptureAllPlotsFromBlueprints()
{
#if WITH_EDITOR

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not get editor world."));
        return;
    }
    else
    {

        FString WorldName = World->GetName();
        UE_LOG(LogTemp, Display, TEXT("Current World Name: %s"), *WorldName);

    }

    // Find CaptureCamera in the level
    ASceneCapture2D* CaptureCamera = nullptr;


    for (TActorIterator<ASceneCapture2D> It(World); It; ++It)
    {
        FString ActorName = It->GetName();
        UE_LOG(LogTemp, Warning, TEXT("Found SceneCapture2D Actor: %s"), *ActorName);

        if (ActorName.Contains(TEXT("SceneCapture2D_UAID_047F0E1DFFCCB35902_1268657983")))
        {
            UE_LOG(LogTemp, Display, TEXT("Using CaptureCamera: %s"), *ActorName);
            CaptureCamera = *It;
            break;
        }
    }

    if (!CaptureCamera)
    {
        UE_LOG(LogTemp, Error, TEXT("CaptureCamera not found in the level."));
        return;
    }

    // Load all plot Blueprints from /Game/BakingTest
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FName FolderPath = TEXT("/Game/BakingTest");
    TArray<FAssetData> Assets;
    AssetRegistry.Get().GetAssetsByPath(FolderPath, Assets, true);

    FName BlueprintClassName = UBlueprint::StaticClass()->GetFName();
    TArray<FAssetData> PlotBlueprints;

    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetClass == BlueprintClassName && Asset.AssetName.ToString().StartsWith("BP_"))
        {
            PlotBlueprints.Add(Asset);
            UE_LOG(LogTemp, Display, TEXT("Found Plot BP: %s"), *Asset.AssetName.ToString());
        }
    }

    if (PlotBlueprints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No Blueprint plots found in: %s"), *FolderPath.ToString());
        return;
    }

    // Loop through each plot blueprint
    for (const FAssetData& Asset : PlotBlueprints)
    {
        FString Name = Asset.AssetName.ToString();
        FStringAssetReference AssetRef(Asset.ToSoftObjectPath().ToString());

        UObject* LoadedObject = AssetRef.TryLoad();
        if (!LoadedObject) continue;

        UBlueprint* BP = Cast<UBlueprint>(LoadedObject);
        if (!BP || !BP->GeneratedClass->IsChildOf<AActor>()) continue;

        //Spawn plot actor
        FActorSpawnParameters SpawnParams;
        AActor* SpawnedPlot = World->SpawnActor<AActor>(BP->GeneratedClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (!SpawnedPlot)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn actor from: %s"), *Name);
            continue;
        }

        // Get SavedWorldPosition variable from actor
        FVector PlotLocation = FVector::ZeroVector;
        FProperty* Prop = SpawnedPlot->GetClass()->FindPropertyByName("SavedWorldPosition");
        if (FStructProperty* VecProp = CastField<FStructProperty>(Prop))
        {
            void* ValuePtr = VecProp->ContainerPtrToValuePtr<void>(SpawnedPlot);
            PlotLocation = *reinterpret_cast<FVector*>(ValuePtr);
        }

        // Move the actor to its position
        SpawnedPlot->SetActorLocation(PlotLocation);

        //Position the camera above the actor
        FVector CamLocation = PlotLocation + FVector(0, 0, 1200); // adjust height as needed
        FRotator CamRotation = FRotator(-90.f, 0.f, 0.f); // Top-down
        CaptureCamera->SetActorLocation(CamLocation);
        CaptureCamera->SetActorRotation(CamRotation);

        //Capture image
        FString OutputPath = FPaths::ProjectSavedDir() + "PlotCaptures/" + Name + ".png";
        UE_LOG(LogTemp, Display, TEXT("Capturing: %s"), *OutputPath);

        UTextureRenderTarget2D* RenderTarget = CaptureCamera->GetCaptureComponent2D()->TextureTarget;
        if (!RenderTarget)
        {
            UE_LOG(LogTemp, Error, TEXT("No RenderTarget assigned to CaptureCamera!"));
            continue;
        }

        FRenderTarget* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
        FIntPoint Size = RTResource->GetSizeXY();

        TArray<FColor> Bitmap;
        RTResource->ReadPixels(Bitmap);

        if (Bitmap.Num() > 0)
        {
            TArray<uint8> PNGData;
            FImageUtils::CompressImageArray(Size.X, Size.Y, Bitmap, PNGData);
            FFileHelper::SaveArrayToFile(PNGData, *OutputPath);
        }

        // Clean up actor
        SpawnedPlot->Destroy();
    }

    UE_LOG(LogTemp, Display, TEXT("Finished capturing all plots."));

#endif
}

ASceneCapture2D* UPlotCaptureUtils::FindCaptureCamera(UWorld* World)
{
    for (TActorIterator<ASceneCapture2D> It(World); It; ++It)
    {
        if (It->GetName().Contains(TEXT("CaptureCamera")))
        {
            return *It;
        }
    }
    return nullptr;
}

FVector UPlotCaptureUtils::GetSavedWorldPosition(AActor* Actor)
{
    FVector Position = FVector::ZeroVector;

    FProperty* Property = Actor->GetClass()->FindPropertyByName(FName("SavedWorldPosition"));
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (StructProp->Struct == TBaseStructure<FVector>::Get())
        {
            void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(Actor);
            Position = *static_cast<FVector*>(ValuePtr);
        }
    }

    return Position;
}

void UPlotCaptureUtils::CaptureSinglePlot(AActor* PlotActor, ASceneCapture2D* CaptureCamera, const FString& SaveDir)
{
    FVector PlotLocation = PlotActor->GetActorLocation();
    FVector CameraOffset(0, 0, 1500.0f);
    FRotator CameraRotation(-90.f, 0.f, 0.f);

    CaptureCamera->SetActorLocation(PlotLocation + CameraOffset);
    CaptureCamera->SetActorRotation(CameraRotation);

    USceneCaptureComponent2D* CaptureComp = CaptureCamera->GetCaptureComponent2D();
    if (!CaptureComp || !CaptureComp->TextureTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid CaptureComponent or RenderTarget."));
        return;
    }

    CaptureComp->CaptureScene();

    FRenderTarget* RenderTarget = CaptureComp->TextureTarget->GameThread_GetRenderTargetResource();
    TArray<FColor> Bitmap;
    RenderTarget->ReadPixels(Bitmap);

    FIntPoint Size = RenderTarget->GetSizeXY(); //FIntPoint Size = CaptureComp->TextureTarget->GetSurfaceSize();
    FString FilePath = SaveDir / (PlotActor->GetName() + TEXT(".png"));

    TArray<uint8> PNGData;
    FImageUtils::CompressImageArray(Size.X, Size.Y, Bitmap, PNGData);
    FFileHelper::SaveArrayToFile(PNGData, *FilePath);

    UE_LOG(LogTemp, Display, TEXT("Saved %s"), *FilePath);
}
