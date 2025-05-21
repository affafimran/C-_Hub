#include "PlotMeshUtils.h"
#include "PlotData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"
#include "Engine/StaticMesh.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "IAssetTools.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshBuilder.h"
#include "ThirdParty/Earcut/earcut.hpp"
#include "PlotMeshUtils.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet/KismetMathLibrary.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableSet.h"
#include "EdGraph/EdGraphPin.h"
#include "ObjectTools.h"
#include "Editor/UnrealEd/Public/ObjectTools.h"
#include "EditorLevelLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "FileHelpers.h"



void UPlotMeshUtils::TriangulatePolygon(const TArray<FVector2D>& Vertices, TArray<int32>& Triangles)
{
    Triangles.Empty();

    int32 NumVertices = Vertices.Num();
    if (NumVertices < 3)
        return;

    TArray<int32> Indices;
    for (int32 i = 0; i < NumVertices; ++i)
        Indices.Add(i);

    int32 Count = 0;
    while (Indices.Num() >= 3 && Count < 5000)
    {
        for (int32 i = 0; i < Indices.Num(); ++i)
        {
            int32 Prev = Indices[(i - 1 + Indices.Num()) % Indices.Num()];
            int32 Curr = Indices[i];
            int32 Next = Indices[(i + 1) % Indices.Num()];

            FVector2D A = Vertices[Prev];
            FVector2D B = Vertices[Curr];
            FVector2D C = Vertices[Next];

            FVector2D AB = B - A;
            FVector2D AC = C - A;

            float Cross = AB.X * AC.Y - AB.Y * AC.X;
            if (Cross <= 0) continue; // Not convex

            bool HasPointInside = false;
            for (int32 j = 0; j < Indices.Num(); ++j)
            {
                if (j == Prev || j == Curr || j == Next) continue;
                FVector2D P = Vertices[Indices[j]];

                FVector2D v0 = C - A;
                FVector2D v1 = B - A;
                FVector2D v2 = P - A;

                float d00 = FVector2D::DotProduct(v0, v0);
                float d01 = FVector2D::DotProduct(v0, v1);
                float d11 = FVector2D::DotProduct(v1, v1);
                float d20 = FVector2D::DotProduct(v2, v0);
                float d21 = FVector2D::DotProduct(v2, v1);

                float denom = d00 * d11 - d01 * d01;
                if (FMath::IsNearlyZero(denom)) continue;

                float u = (d11 * d20 - d01 * d21) / denom;
                float v = (d00 * d21 - d01 * d20) / denom;

                if (u >= 0 && v >= 0 && (u + v) <= 1)
                {
                    HasPointInside = true;
                    break;
                }
            }

            if (HasPointInside) continue;

            Triangles.Add(Prev);
            Triangles.Add(Next);
            Triangles.Add(Curr);
            
            Indices.RemoveAt(i);
            break;
        }

        Count++;
    }
}

//Earcut Library

struct FEarcutVec2
{
    float X;
    float Y;

    FEarcutVec2(float InX, float InY)
        : X(InX), Y(InY)
    {}
};

namespace mapbox {
    namespace util {
        template <>
        struct nth<0, FEarcutVec2> {
            static float get(const FEarcutVec2& v) { return v.X; }
        };
        template <>
        struct nth<1, FEarcutVec2> {
            static float get(const FEarcutVec2& v) { return v.Y; }
        };
    }
}

void UPlotMeshUtils::EarcutTriangulate(const TArray<FVector2D>& Vertices, TArray<int32>& Triangles)
{
    Triangles.Empty();

    if (Vertices.Num() < 3) return;

    // Convert to format expected by Earcut
    std::vector<std::vector<FEarcutVec2>> Polygon;
    std::vector<FEarcutVec2> OuterRing;

    for (const FVector2D& Pt : Vertices)
    {
        OuterRing.push_back(FEarcutVec2(Pt.X, Pt.Y));
        //OuterRing.push_back({ Pt.X, Pt.Y });
    }

    Polygon.push_back(OuterRing);

    std::vector<uint32_t> Indices = mapbox::earcut<uint32_t>(Polygon);

    for (uint32_t Idx : Indices)
    {
        Triangles.Add(static_cast<int32>(Idx));
    }
}

//

#pragma region CreateMeshOnly

bool UPlotMeshUtils::SavePlotToRegistry(
    UProceduralMeshComponent* ProcMesh,
    const FString& AssetName,
    const FString& FolderPath,
    const FVector& WorldLocation,
    UMaterialInterface* Material,
    UDataTable* RegistryTable
)
{
    if (!ProcMesh || !RegistryTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid input to SavePlotToRegistry."));
        return false;
    }

    FString CleanFolder = FolderPath;
    CleanFolder.RemoveFromStart(TEXT("/Game"));
    CleanFolder.RemoveFromStart(TEXT("Game"));

    FString ObjectPath = FString::Printf(TEXT("/Game/%s/%s.%s"), *CleanFolder, *AssetName, *AssetName);

    if (!SaveProceduralMeshAsStaticMesh(ProcMesh, AssetName, CleanFolder, Material))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save procedural mesh."));
        return false;
    }

    UStaticMesh* StaticMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *ObjectPath));
    if (!StaticMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load saved static mesh from path: %s"), *ObjectPath);
        return false;
    }

    // Material re-assign (redundant, but safe)
    if (Material)
    {
        StaticMesh->Modify();
        StaticMesh->GetStaticMaterials().Empty();
        StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Material));
        StaticMesh->SetMaterial(0, Material);
        StaticMesh->MarkPackageDirty();
        StaticMesh->PostEditChange();
    }

    FPlotMeshEntry NewEntry;
    NewEntry.Mesh = StaticMesh;
    NewEntry.WorldLocation = WorldLocation;
    NewEntry.Material = Material;

    FName RowName(*AssetName);
    RegistryTable->AddRow(RowName, NewEntry);

    if (const FPlotMeshEntry* RowCheck = RegistryTable->FindRow<FPlotMeshEntry>(RowName, TEXT("Check")))
    {
        UE_LOG(LogTemp, Warning, TEXT("Row added: %s — Mesh: %s, Pos: %s"),
            *RowName.ToString(),
            *GetNameSafe(RowCheck->Mesh),
            *RowCheck->WorldLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Row NOT added."));
    }

    RegistryTable->MarkPackageDirty();
    RegistryTable->PostEditChange();
    FAssetRegistryModule::AssetCreated(RegistryTable);

    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(RegistryTable->GetOutermost());
    FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, true, false);

    return true;
}
#pragma endregion


#pragma region CreateBP


//BP Actor

class USimpleConstructionScript;

bool UPlotMeshUtils::SavePlotAsActor(
    UProceduralMeshComponent* ProcMesh,
    const FString& ActorName,
    const FString& FolderPath,
    const FVector& WorldLocation,
    UMaterialInterface* Material)
{
#if WITH_EDITOR
    FString MeshName = ActorName + TEXT("_Mesh");
    FString MeshFolder = FolderPath + TEXT("/Meshes");
    if (!SaveProceduralMeshAsStaticMesh(ProcMesh, MeshName, MeshFolder, Material))
        return false;

    FString MeshPath = MeshFolder + TEXT("/") + MeshName + TEXT(".") + MeshName;
    UStaticMesh* StaticMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *MeshPath));
    if (!StaticMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load saved StaticMesh at path: %s"), *MeshPath);
        return false;
    }

    FString BlueprintPath = FolderPath / (TEXT("BP_") + ActorName);
    UPackage* Package = CreatePackage(*BlueprintPath);
    if (!Package)
        return false;

    UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(),
        Package,
        FName(*FString("BP_" + ActorName)),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass()
    );

    if (!NewBP) { return false; }

    
    // Create StaticMeshComponent
    USCS_Node* SMNode = NewBP->SimpleConstructionScript->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("PlotMesh"));
    NewBP->SimpleConstructionScript->AddNode(SMNode);

    if (UStaticMeshComponent* Template = Cast<UStaticMeshComponent>(SMNode->ComponentTemplate))
    {
        Template->SetStaticMesh(StaticMesh);
        Template->SetRelativeLocation(FVector::ZeroVector);
        if (Material)
        {
            Template->SetMaterial(0, Material);
        }
    }

    // Add FVector variable "SavedWorldPosition"
    FEdGraphPinType PosType;
    PosType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    PosType.PinSubCategoryObject = TBaseStructure<FVector>::Get();

    FBPVariableDescription NewVar;
    NewVar.VarName = "SavedWorldPosition";
    NewVar.VarGuid = FGuid::NewGuid();
    NewVar.VarType = PosType;
    NewVar.PropertyFlags = CPF_Edit | CPF_BlueprintVisible;
    NewVar.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
    NewBP->NewVariables.Add(NewVar);

    // Add bool: bSnapToSavedPosition
    FBPVariableDescription BoolVar;
    BoolVar.VarName = "bSnapToSavedPosition";
    BoolVar.VarGuid = FGuid::NewGuid();
    BoolVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    BoolVar.PropertyFlags = CPF_Edit | CPF_BlueprintVisible;
    BoolVar.DefaultValue = TEXT("true"); // Default enabled for one-time snap
    NewBP->NewVariables.Add(BoolVar);


//    #pragma region Construction_Script_Logic
//    //UEdGraph* CSGraph = FBlueprintEditorUtils::FindUserConstructionScript(NewBP);
//    if (!CSGraph)
//    {
//        UE_LOG(LogTemp, Error, TEXT("CSGraph is null — Construction script not created properly!"));
//        return false;
//    }
//
//    if (CSGraph)
//    {
//        UEdGraphNode* EntryNode = nullptr;
//        for (UEdGraphNode* Node : CSGraph->Nodes)
//        {
//            if (Node->IsA<UK2Node_FunctionEntry>())
//            {
//                EntryNode = Node;
//                break;
//            }
//        }
//
//        if (EntryNode)
//        {
//            #pragma region Plot_Placement_bool
//
//            // 1. Add: Get bSnapToSavedPosition
//            UK2Node_VariableGet* GetBoolNode = NewObject<UK2Node_VariableGet>(CSGraph);
//            GetBoolNode->VariableReference.SetSelfMember("bSnapToSavedPosition");
//            CSGraph->AddNode(GetBoolNode);
//            GetBoolNode->AllocateDefaultPins();
//            GetBoolNode->NodePosX = 200;
//
//            // 2. Add: Branch node
//            UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(CSGraph);
//            CSGraph->AddNode(BranchNode);
//            BranchNode->AllocateDefaultPins();
//            BranchNode->NodePosX = 400;
//
//            // 3. Connect Entry → Branch Exec
//            if (UK2Node_FunctionEntry* EntryFunc = Cast<UK2Node_FunctionEntry>(EntryNode))
//            {
//                UEdGraphPin* EntryExecPin = EntryFunc->FindPin(TEXT("Then"));
//                UEdGraphPin* BranchExecPin = BranchNode->GetExecPin();
//
//                if (EntryExecPin && BranchExecPin)
//                {
//                    EntryExecPin->MakeLinkTo(BranchExecPin);
//                }
//            }
//
//            // 4. Connect Bool → Branch Condition
//            GetBoolNode->GetValuePin()->MakeLinkTo(BranchNode->GetConditionPin());
//
//            // 5. Add Get SavedWorldPosition
//            UK2Node_VariableGet* GetSavedPos_Node = NewObject<UK2Node_VariableGet>(
//                NewBP, // Use Blueprint as Outer, not CSGraph
//                UK2Node_VariableGet::StaticClass()
//            );
//            CSGraph->AddNode(GetSavedPos_Node);
//            GetSavedPos_Node->CreateNewGuid();
//            GetSavedPos_Node->PostPlacedNewNode();
//            GetSavedPos_Node->AllocateDefaultPins();
//            GetSavedPos_Node->AutowireNewNode(nullptr);
//            GetSavedPos_Node->NodePosX = 600;
//            GetSavedPos_Node->NodePosY = 200;
//            GetSavedPos_Node->VariableReference.SetSelfMember("SavedWorldPosition");
//
//            // 6. Add SetActorLocation
//            UK2Node_CallFunction* SetActorLocation_Node = NewObject<UK2Node_CallFunction>(CSGraph);
//            SetActorLocation_Node->SetFromFunction(AActor::StaticClass()->FindFunctionByName("K2_SetActorLocation"));
//            CSGraph->AddNode(SetActorLocation_Node);
//            SetActorLocation_Node->AllocateDefaultPins();
//            SetActorLocation_Node->NodePosX = 900;
//
//            // 7. Wire Branch THEN → SetActorLocation Exec
//            BranchNode->GetThenPin()->MakeLinkTo(SetActorLocation_Node->GetExecPin());
//
//            // 8. Wire SavedWorldPosition → NewLocation input
//            UEdGraphPin* SetActorLocationInputPin = SetActorLocation_Node->FindPin(TEXT("NewLocation"));
//            if (SetActorLocationInputPin)
//            {
//                SetActorLocationInputPin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
//            }
//
//            // 9. Add Set bSnapToSavedPosition = false
//            UK2Node_VariableSet* SetBoolNode = NewObject<UK2Node_VariableSet>(CSGraph);
//            SetBoolNode->VariableReference.SetSelfMember("bSnapToSavedPosition");
//            CSGraph->AddNode(SetBoolNode);
//            SetBoolNode->AllocateDefaultPins();
//            SetBoolNode->NodePosX = 1150;
//
//            SetBoolNode->GetValuePin()->DefaultValue = TEXT("false");
//
//            // 10. Wire SetActorLocation → Set bSnapToSavedPosition
//            SetActorLocation_Node->GetThenPin()->MakeLinkTo(SetBoolNode->GetExecPin());
//
//#pragma endregion
//        }
//    }
//
//
//#pragma endregion

#pragma region Junk_Code
    //// Construction script logic///////////////////////////////////////////
    UEdGraph* CSGraph = FBlueprintEditorUtils::FindUserConstructionScript(NewBP);
    
    if (CSGraph)
    {
        UEdGraphNode* EntryNode = nullptr;
        for (UEdGraphNode* Node : CSGraph->Nodes)
        {
            if (Node->IsA<UK2Node_FunctionEntry>())
            {
                EntryNode = Node;
                break;
            }
        }

        if (EntryNode)
        {
            UK2Node_VariableGet* GetSavedPos = NewObject<UK2Node_VariableGet>(CSGraph);
            GetSavedPos->VariableReference.SetSelfMember("SavedWorldPosition");
            CSGraph->AddNode(GetSavedPos);
            GetSavedPos->AllocateDefaultPins();

            UK2Node_CallFunction* SetLoc = NewObject<UK2Node_CallFunction>(CSGraph);
            SetLoc->SetFromFunction(AActor::StaticClass()->FindFunctionByName("K2_SetActorLocation"));
            CSGraph->AddNode(SetLoc);
            SetLoc->AllocateDefaultPins();

            // Visual position (optional)
            EntryNode->NodePosX = 0;
            GetSavedPos->NodePosX = 300;
            SetLoc->NodePosX = 600;

            // Wire exec: Entry → SetActorLocation
            if (UK2Node_FunctionEntry* EntryFunc = Cast<UK2Node_FunctionEntry>(EntryNode))
            {
                UEdGraphPin* EntryExecPin = EntryFunc->FindPin(TEXT("Then"));
                UEdGraphPin* SetLocExecPin = SetLoc->GetExecPin();

                if (EntryExecPin && SetLocExecPin)
                {
                    EntryExecPin->MakeLinkTo(SetLocExecPin);
                }
            }

            // Wire data: SavedWorldPosition → NewLocation
            FVector LiteralLocation = WorldLocation;

            FString LiteralValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), LiteralLocation.X, LiteralLocation.Y, LiteralLocation.Z);

            UEdGraphPin* SetLocInput = nullptr;
            for (UEdGraphPin* Pin : SetLoc->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input && Pin->PinName == TEXT("NewLocation"))
                {
                    SetLocInput = Pin;
                    break;
                }
            }

            if (SetLocInput)
            {
                SetLocInput->DefaultValue = LiteralValue;
                UE_LOG(LogTemp, Display, TEXT("SetActorLocation.NewLocation default value set to %s"), *LiteralValue);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Could not find NewLocation input pin on SetActorLocation!"));
            }


            //#pragma region Plot_Placement_bool
            //// 1. Add: Get bSnapToSavedPosition
            //UK2Node_VariableGet* GetBoolNode = NewObject<UK2Node_VariableGet>(CSGraph);
            //GetBoolNode->VariableReference.SetSelfMember("bSnapToSavedPosition");
            //CSGraph->AddNode(GetBoolNode);
            //GetBoolNode->AllocateDefaultPins();
            //GetBoolNode->NodePosX = 300;

            //// 2. Add: Branch node
            //UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(CSGraph);
            //CSGraph->AddNode(BranchNode);
            //BranchNode->AllocateDefaultPins();
            //BranchNode->NodePosX = 450;

            //// 3. Connect Entry → Branch Exec
            //if (UK2Node_FunctionEntry* EntryFunc = Cast<UK2Node_FunctionEntry>(EntryNode))
            //{
            //    UEdGraphPin* EntryExecPin = EntryFunc->FindPin(TEXT("Then"));
            //    UEdGraphPin* BranchExecPin = BranchNode->GetExecPin();

            //    if (EntryExecPin && BranchExecPin)
            //    {
            //        EntryExecPin->MakeLinkTo(BranchExecPin);
            //    }
            //}

            //// 4. Connect Bool → Branch Condition
            //GetBoolNode->GetValuePin()->MakeLinkTo(BranchNode->GetConditionPin());

            //// 5. Add Get SavedWorldPosition
            //UK2Node_VariableGet* GetSavedPos_Node = NewObject<UK2Node_VariableGet>(CSGraph);
            //GetSavedPos_Node->VariableReference.SetSelfMember("SavedWorldPosition");
            //CSGraph->AddNode(GetSavedPos_Node);
            //GetSavedPos_Node->AllocateDefaultPins();
            //GetSavedPos_Node->NodePosX = 600;

            //// 6. Add SetActorLocation
            //UK2Node_CallFunction* SetActorLocation_Node = NewObject<UK2Node_CallFunction>(CSGraph);
            //SetActorLocation_Node->SetFromFunction(AActor::StaticClass()->FindFunctionByName("K2_SetActorLocation"));
            //CSGraph->AddNode(SetActorLocation_Node);
            //SetActorLocation_Node->AllocateDefaultPins();
            //SetActorLocation_Node->NodePosX = 900;

            //// 7. Wire Branch THEN → SetActorLocation Exec
            //BranchNode->GetThenPin()->MakeLinkTo(SetActorLocation_Node->GetExecPin());

            //// 8. Wire SavedWorldPosition → SetActorLocation NewLocation
            //UEdGraphPin* SetActorLocationInputPin = SetActorLocation_Node->FindPin(TEXT("NewLocation"));
            //if (SetActorLocationInputPin)
            //{
            //    SetActorLocationInputPin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
            //}

            //// 9. Add Set bSnapToSavedPosition = false
            //UK2Node_VariableSet* SetBoolNode = NewObject<UK2Node_VariableSet>(CSGraph);
            //SetBoolNode->VariableReference.SetSelfMember("bSnapToSavedPosition");
            //CSGraph->AddNode(SetBoolNode);
            //SetBoolNode->AllocateDefaultPins();
            //SetBoolNode->NodePosX = 1150;

            //SetBoolNode->GetValuePin()->DefaultValue = TEXT("false");

            //// 10. Wire SetActorLocation → Set bSnapToSavedPosition
            //SetActorLocation_Node->GetThenPin()->MakeLinkTo(SetBoolNode->GetExecPin());

            //#pragma endregion

            //
            /*UEdGraphPin* SavedPosOutput = GetSavedPos->GetValuePin();
            UEdGraphPin* SetLocInput = SetLoc->FindPin(TEXT("NewLocation"));

            if (SavedPosOutput && SetLocInput)
            {
                SavedPosOutput->MakeLinkTo(SetLocInput);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Wiring failed: one or more pins were null!"));
            }*/
            //
        }
    }
    ////////////////////////////////////////////////////////////////////


#pragma endregion


    // Finalization
    FAssetRegistryModule::AssetCreated(NewBP);
    Package->MarkPackageDirty();
    NewBP->PostEditChange();

    FBlueprintEditorUtils::RefreshAllNodes(NewBP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(NewBP);
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    // Save Package to disk
    FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

    bool bSaved = UPackage::SavePackage(
        Package,
        nullptr,
        EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
        *PackageFileName,
        GError,
        nullptr,
        false,
        true,
        SAVE_NoError
    );

    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save BP package: %s"), *PackageFileName);
        return false;
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Successfully saved BP actor: %s"), *PackageFileName);
    }

    return true;
#else
    return false;
#endif
}

#pragma endregion
//




bool UPlotMeshUtils::LoadPlotDataFromFile(const FString& FilePath, TArray<FPlotData>& OutPlots)
{
    OutPlots.Empty();

    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *FilePath))
        return false;

    TSharedPtr<FJsonValue> RootValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);

    if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
        return false;

    const TArray<TSharedPtr<FJsonValue>>* PlotArray;
    if (!RootValue->TryGetArray(PlotArray))
        return false;

    for (const TSharedPtr<FJsonValue>& PlotVal : *PlotArray)
    {
        const TSharedPtr<FJsonObject>* PlotObj;
        if (!PlotVal->TryGetObject(PlotObj))
            continue;

        FPlotData Plot;

        Plot.Name = PlotObj->Get()->GetStringField("name");

        const TArray<TSharedPtr<FJsonValue>>* PointsArray;
        if (PlotObj->Get()->TryGetArrayField("points", PointsArray))
        {
            for (const TSharedPtr<FJsonValue>& PtVal : *PointsArray)
            {
                const TArray<TSharedPtr<FJsonValue>>* Coord;
                if (PtVal->TryGetArray(Coord) && Coord->Num() == 2)
                {
                    float X = static_cast<float>((*Coord)[0]->AsNumber());
                    float Y = static_cast<float>((*Coord)[1]->AsNumber());
                    Plot.Points.Add(FVector2D(X, Y));
                }
            }
        }

        OutPlots.Add(Plot);
    }

    return true;
}

bool UPlotMeshUtils::SaveProceduralMeshAsStaticMesh(UProceduralMeshComponent* ProcMesh,const FString& AssetName,const FString& FolderPath,UMaterialInterface* Material)
{
#if WITH_EDITOR
    if (!ProcMesh || ProcMesh->GetNumSections() == 0)
        return false;

    const FProcMeshSection* Section = ProcMesh->GetProcMeshSection(0);
    if (!Section || Section->ProcVertexBuffer.IsEmpty() || Section->ProcIndexBuffer.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ERROR: Section 0 is empty! Mesh not saved."));
        return false;
    }

    // Clean folder path
    FString CleanFolder = FolderPath;
    CleanFolder.RemoveFromStart(TEXT("/Game"));
    CleanFolder.RemoveFromStart(TEXT("Game"));

    // Build full asset path
    FString PackagePath = FString::Printf(TEXT("/Game/%s/%s"), *CleanFolder, *AssetName);
    FString PackageFilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackagePath);
        return false;
    }

    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!StaticMesh) return false;

    // Build mesh data
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
        Vertices.Add(Vertex.Position);

    for (uint32 Index : Section->ProcIndexBuffer)
        Triangles.Add(static_cast<int32>(Index));

    FMeshDescription MeshDesc;
    FStaticMeshAttributes Attributes(MeshDesc);
    Attributes.Register();

    TMap<int32, FVertexID> VertexMap;
    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        FVertexID VertexID = MeshDesc.CreateVertex();
        Attributes.GetVertexPositions()[VertexID] = FVector3f(Vertices[i]);
        VertexMap.Add(i, VertexID);
    }

    FPolygonGroupID PolygonGroup = MeshDesc.CreatePolygonGroup();

    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        TArray<FVertexInstanceID> Verts;
        for (int32 j = 0; j < 3; ++j)
        {
            FVertexID VertexID = VertexMap[Triangles[i + j]];
            FVertexInstanceID InstanceID = MeshDesc.CreateVertexInstance(VertexID);
            Verts.Add(InstanceID);
        }
        MeshDesc.CreatePolygon(PolygonGroup, Verts);
    }

    StaticMesh->SetNumSourceModels(1);
    StaticMesh->CreateMeshDescription(0, MeshDesc);
    StaticMesh->CommitMeshDescription(0);

    // Materials
    if (Material)
    {
        StaticMesh->SetStaticMaterials({ FStaticMaterial(Material) });
        UE_LOG(LogTemp, Warning, TEXT("Material assigned to Static Mesh."));
    }

    StaticMesh->GetSourceModel(0).BuildSettings.bRecomputeNormals = true;
    StaticMesh->GetSourceModel(0).BuildSettings.bRecomputeTangents = true;
    StaticMesh->Build(false);
    StaticMesh->PostEditChange();
    StaticMesh->MarkPackageDirty();

    FAssetRegistryModule::AssetCreated(StaticMesh);

    // Save to disk
    bool bSaved = UPackage::SavePackage(Package, StaticMesh, EObjectFlags::RF_Public | RF_Standalone, *PackageFilePath, GError, nullptr, false, true, SAVE_NoError);

    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save StaticMesh package: %s"), *PackageFilePath);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("StaticMesh saved successfully to: %s"), *PackageFilePath);
    return true;
#else
    return false;
#endif
}

