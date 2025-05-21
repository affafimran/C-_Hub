// ConstructionScriptCleaner.cpp

#include "UConstructionScriptCleaner.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_Variable.h"


void UConstructionScriptCleaner::ClearConstructionScriptsInFolder(const FString& FolderPath)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> AssetList;
    FARFilter Filter;
    Filter.PackagePaths.Add(*FolderPath);
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;

    AssetRegistryModule.Get().GetAssets(Filter, AssetList);

    int32 ModifiedCount = 0;

    for (const FAssetData& AssetData : AssetList)
    {
        UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
        if (!Blueprint || !Blueprint->GeneratedClass)
        {
            continue;
        }

        // Ensure it's an Actor-based Blueprint
        if (!Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
        {
            continue;
        }

        USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
        if (!SCS)
        {
            continue;
        }

        const TArray<USCS_Node*>& Nodes = SCS->GetAllNodes();
        if (Nodes.Num() == 0)
        {
            continue;
        }

        // Remove all nodes
        for (USCS_Node* Node : Nodes)
        {
            SCS->RemoveNode(Node);
        }

        // Mark dirty, compile, and save
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        UEditorAssetLibrary::SaveAsset(AssetData.ObjectPath.ToString(), false);
        UE_LOG(LogTemp, Warning, TEXT("Cleared CS for: %s"), *Blueprint->GetName());
        ModifiedCount++;
    }

    UE_LOG(LogTemp, Warning, TEXT("Done. Modified %d Blueprints."), ModifiedCount);
}
void UConstructionScriptCleaner::ClearConstructionScriptGraphOnly(const FString& FolderPath)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.PackagePaths.Add(*FolderPath);
    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssets(Filter, Assets);

    UE_LOG(LogTemp, Warning, TEXT("Scanning folder: %s"), *FolderPath);
    UE_LOG(LogTemp, Warning, TEXT("Assets found: %d"), Assets.Num());

    int32 Modified = 0;

    for (const FAssetData& AssetData : Assets)
    {
        UObject* Asset = AssetData.GetAsset();
        UBlueprint* Blueprint = Cast<UBlueprint>(Asset);

        if (!Blueprint || !Blueprint->GeneratedClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipping (not a Blueprint or missing class): %s"), *AssetData.AssetName.ToString());
            continue;
        }

        if (!Blueprint->GeneratedClass->IsChildOf<AActor>())
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipping (not an Actor): %s"), *Blueprint->GetName());
            continue;
        }

        // Find the Construction Script graph
        UEdGraph* CSGraph = nullptr;
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph && Graph->GetName() == TEXT("ConstructionScript"))
            {
                CSGraph = Graph;
                break;
            }
        }

        if (!CSGraph || CSGraph->Nodes.Num() == 0)
            continue;

        UE_LOG(LogTemp, Warning, TEXT("Cleaning Construction Script for: %s"), *Blueprint->GetName());

        TArray<UEdGraphNode*> NodesToRemove;

        for (UEdGraphNode* Node : CSGraph->Nodes)
        {
            if (!Node)
                continue;

            const FString NodeClass = Node->GetClass()->GetName();
            const FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

            UE_LOG(LogTemp, Warning, TEXT("Node: %s (%s)"), *NodeTitle, *NodeClass);

            // Keep entry, variable get/set, comments
            if (Node->IsA<UK2Node_FunctionEntry>() ||
                Node->IsA<UK2Node_Variable>() ||
                NodeClass.Contains(TEXT("EdGraphNode_Comment")))
            {
                continue;
            }

            NodesToRemove.Add(Node);
        }

        if (NodesToRemove.Num() == 0)
            continue;

        for (UEdGraphNode* Node : NodesToRemove)
        {
            CSGraph->RemoveNode(Node);
        }

        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        UEditorAssetLibrary::SaveAsset(AssetData.ObjectPath.ToString(), false);

        UE_LOG(LogTemp, Warning, TEXT("Cleaned: %s (Removed %d node(s))"), *Blueprint->GetName(), NodesToRemove.Num());
        Modified++;
    }

    UE_LOG(LogTemp, Warning, TEXT("Finished. Total Blueprints cleaned: %d"), Modified);
}