// Fill out your copyright notice in the Description page of Project Settings.


#include "ConstructionScriptLinkBreaker.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "Engine/Selection.h"

void UConstructionScriptLinkBreaker::BreakFunctionLinksFromConstructionScripts(const TArray<AActor*>& SelectedActors, FName TargetFunctionName)
{
    for (AActor* Actor : SelectedActors)
    {
        if (!Actor) continue;

        UBlueprint* Blueprint = Cast<UBlueprint>(Actor->GetClass()->ClassGeneratedBy);
        if (!Blueprint) continue;

        UEdGraph* ConstructionGraph = FBlueprintEditorUtils::FindUserConstructionScript(Blueprint);
        if (!ConstructionGraph) continue;

        bool bGraphModified = false;

        for (UEdGraphNode* Node : ConstructionGraph->Nodes)
        {
            if (!Node) continue;

            if (UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
            {
                if (CallFunctionNode->GetFunctionName() == TargetFunctionName)
                {
                     UE_LOG(LogTemp, Warning, TEXT("Found Function Node: %s"), *CallFunctionNode->GetFunctionName().ToString());
                    for (UEdGraphPin* Pin : CallFunctionNode->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                        {
                            Pin->BreakAllPinLinks();
                            bGraphModified = true;

                            UE_LOG(LogTemp, Warning, TEXT("Broke links from function '%s' in Blueprint '%s'."),
                                *TargetFunctionName.ToString(), *Blueprint->GetName());
                        }
                    }
                }
            }
        }

        if (bGraphModified)
        {
            Blueprint->Modify();
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);
            UE_LOG(LogTemp, Warning, TEXT("Modified and recompiled Blueprint: %s"), *Blueprint->GetName());
        }
    }
}
