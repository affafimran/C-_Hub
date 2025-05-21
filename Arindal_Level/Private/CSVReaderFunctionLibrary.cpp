// Fill out your copyright notice in the Description page of Project Settings.


#include "CSVReaderFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

bool UCSVReaderFunctionLibrary::LoadFileToLines(const FString& FilePath, TArray<FString>& OutLines)
{
    OutLines.Empty();

    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("CSV file not found: %s"), *FilePath);
        return false;
    }

    bool bSuccess = FFileHelper::LoadFileToStringArray(OutLines, *FilePath);

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load CSV file: %s"), *FilePath);
    }

    return bSuccess;
}
