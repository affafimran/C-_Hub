#include "RenderCaptureUtils.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

bool URenderCaptureUtils::SaveRenderTargetToPNG(UTextureRenderTarget2D* RenderTarget, const FString& FileName, const FString& Directory)
{
#if !UE_BUILD_SHIPPING
    if (!RenderTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("RenderTarget is null."));
        return false;
    }

    // Read pixels from render target
    FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
    if (!RTResource)
    {
        UE_LOG(LogTemp, Error, TEXT("RenderTarget resource is invalid."));
        return false;
    }

    TArray<FColor> Bitmap;
    RTResource->ReadPixels(Bitmap);

    int32 Width = RenderTarget->SizeX;
    int32 Height = RenderTarget->SizeY;

    // Flip image vertically
    /*for (int32 Row = 0; Row < Height / 2; ++Row)
    {
        int32 IndexA = Row * Width;
        int32 IndexB = (Height - 1 - Row) * Width;
        for (int32 Col = 0; Col < Width; ++Col)
        {
            Bitmap.SwapMemory(IndexA + Col, IndexB + Col);
        }
    }*/

    // Create screenshot folder
    FString Folder = Directory.IsEmpty()
        ? FPaths::ProjectSavedDir() / TEXT("Screenshots/Captures")
        : Directory;


    // Step 2: Normalize and convert to absolute
    FPaths::NormalizeDirectoryName(Folder);
    FPaths::MakePlatformFilename(Folder);
    FString FullFolderPath = FPaths::ConvertRelativePathToFull(Folder);

    // Step 3: Ensure folder exists
    IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
    if (!FileManager.DirectoryExists(*Folder))
    {
        FileManager.CreateDirectoryTree(*Folder);
    }

    FString CleanName = FPaths::GetCleanFilename(FileName);
    CleanName = FPaths::MakeValidFileName(FileName);
    FString PNGPath = FullFolderPath / (FPaths::GetBaseFilename(CleanName, false) + TEXT(".png"));
    FPaths::NormalizeFilename(PNGPath);

    // Compress & save PNG
    TArray<uint8> PNGData;
    FImageUtils::CompressImageArray(Width, Height, Bitmap, PNGData);
    bool bSaved = FFileHelper::SaveArrayToFile(PNGData, *PNGPath);

    if (bSaved)
    {
        UE_LOG(LogTemp, Display, TEXT("Saved render target to: %s"), *PNGPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save PNG: %s"), *PNGPath);
    }

    return bSaved;
#else
    return false;
#endif
}
