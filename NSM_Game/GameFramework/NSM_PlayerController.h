#pragma once

#include "CoreMinimal.h"
#include "NSM_BaseHUD.h"
#include "GameFramework/PlayerController.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"
#include "NordicSimMap/UI/NSM_BaseWidget.h"
#include "NordicSimMap/UI/RadialWheel/NSM_RadialMenuWheelBase.h"
#include "NSM_PlayerController.generated.h"


class ANSM_SpawnArea;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSM_IsInRTSMode, bool, IsInRTSMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSM_SetCursor, EMouseCursor::Type, MouseCursor);

class UNSM_RTSControllerComponent;
class UNSM_InteractionComponent;
class UNSM_EquipmentComponent;

UCLASS()
class NORDICSIMMAP_API ANSM_PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ANSM_PlayerController();
	
	/** Setup input actions and context mappings for player. */
	virtual void SetupInputComponent() override;
	
	/** Mapping context used for pawn control. */
	UPROPERTY()
	class UInputMappingContext* PawnMappingContext;

	/** Action to update location. */
	UPROPERTY()
	class UInputAction* MoveAction;

	/** Action to update location. */
	UPROPERTY()
	class UInputAction* MoveUpAction;

	/** Action to update location. */
	UPROPERTY()
	class UInputAction* IncrementSpeedAction;

	/** Action to update location. */
	UPROPERTY()
	class UInputAction* CancelAction;

	/** Action to update forward movement. */
	UPROPERTY()
	class UInputAction* MoveForwardAction;

	/** Action to update right movement. */
	UPROPERTY()
	class UInputAction* MoveRightAction;

	/** Action to SwitchWeapons. */
	UPROPERTY()
	class UInputAction* SwitchWeaponAction;

	/** Action to SwitchWeapons with wheal. */
	UPROPERTY()
	class UInputAction* SwitchWeaponWheelAction;

	/** Action to update rotation. */
	UPROPERTY()
	class UInputAction* VerticalRotateAction;

	/** Action to update zoom. */
	UPROPERTY()
	class UInputAction* ZoomAction;
	
	/** Action to update rotation. */
	UPROPERTY()
	class UInputAction* HorizontalRotateAction;

	/** Action to check left click (for RTS Selection). */
	UPROPERTY()
	class UInputAction* LeftMouseClick;

	/** Action to check right click (for RTS Selection). */
	UPROPERTY()
	class UInputAction* RightMouseClick;

	/** Action to check right click (for RTS Selection). */
	UPROPERTY()
	class UInputAction* LeftAltButton;

	/** Action to check right click (for RTS Selection). */
	UPROPERTY()
	class UInputAction* RightAltButton;
	
	/** Action to check right click (for RTS Selection). */
	UPROPERTY()
	class UInputAction* BackSpaceButton;

	/** Action to spawn and throw a grenade (Fire Bomb). */
	UPROPERTY()
	class UInputAction* GrenadeAction;
	
	/** Action to drop item. */
	UPROPERTY()
	class UInputAction* DropItemAction;
	
	/** Action to drop item. */
	UPROPERTY()
	class UInputAction* AimDownSightAction;	
	
	/** Action to sprint */
	UPROPERTY()
	class UInputAction* SprintAction;
	
	UPROPERTY()
	class UInputAction* CrouchAction;
	
	/** Action to drop item. */
	UPROPERTY()
	class UInputAction* StopFormationAction;
	
	/** Action to drop item. */
	UPROPERTY()
	class UInputAction* ChangeFormationAction;
	
	/** Action to drop item. */
	UPROPERTY()
	class UInputAction* SplitFormationAction;
	
	/** Action to make the camera jump between locations. */
	UPROPERTY()
	class UInputAction* RTSCameraSteppingModeAction;
	
	UPROPERTY()
	class UInputAction* PossessSelectedPawn;
	
	UPROPERTY()
	class UInputAction* RevertSquadOrdersAction;
	
	UPROPERTY()
	class UInputAction* MoveToOrderAction;
	
	UPROPERTY()
	class UInputAction* SquadOneAction;
	UPROPERTY()
	class UInputAction* SquadTwoAction;
	UPROPERTY()
	class UInputAction* SquadThreeAction;	
	UPROPERTY()
	class UInputAction* SquadFourAction;	
	UPROPERTY()
	class UInputAction* SquadFiveAction;	
	UPROPERTY()
	class UInputAction* SquadSixAction;	
	UPROPERTY()
	class UInputAction* SquadSevenAction;

	UPROPERTY(EditAnywhere)
	UNSM_FormationManagerComponent* FormationManagerComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNSM_InteractionComponent* InteractionComponent;
	
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Widget")
	TSubclassOf<UNSM_BaseWidget> RTSWidgetClass;
	
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	TArray<ANSM_CharacterBase*> ActorsSelected;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TArray<UNSM_SquadActionDataAsset*> GeneralSquadActions;
	
	UPROPERTY(BlueprintAssignable)
	FNSM_IsInRTSMode InRTSModeEvent;
	
	UPROPERTY(BlueprintAssignable)
	FNSM_SetCursor SetCursorDelegate;

	/** boolean to know if the player is or not in rts mode. */
	UPROPERTY(ReplicatedUsing = OnRep_InRTSMode)
	bool bInRTSMode;

	UFUNCTION()
	void OnRep_InRTSMode();

	/** boolean to know if the player is or not in rts mode. */
	UPROPERTY()
	FTimerHandle CameraTransitionTimerHandle;
	
	/** Vector that represents the initial point of the selection box. */
	UPROPERTY()
	FVector2D InitialSelectionPoint;

	/** Vector that represents the current point of the selection box. */
	UPROPERTY()
	FVector2D CurrentSelectionPoint;

	UFUNCTION(BlueprintCallable)
	void OpenRadialWheel();
	
	UFUNCTION(BlueprintCallable)
	ANSM_CharacterBase* GetCharacterReference() const { return CharacterRef; }
	
	void SetCharacterReference(ANSM_CharacterBase* InCharacterRef) { CharacterRef = InCharacterRef;}

	UFUNCTION(Server, Reliable)
	void Server_SetCharacterReference(ANSM_CharacterBase* InCharacterRef);

	UFUNCTION(BlueprintCallable)
	ANSM_RTSPawn* GetRTSPawnReference() const {return RTSPawnRef;}
	
	void SetRTSPawnReference(ANSM_RTSPawn* InRtsPawnRef) { RTSPawnRef = InRtsPawnRef;}
	

	UFUNCTION(BlueprintCallable)
	void CloseRadialWheel();
	
	UFUNCTION()
	void InitializePlayer(const int32 PlayerAmount);
	
	UFUNCTION()
	void EndSelectionBox() const;

	UFUNCTION()
	void UpdateSelectionBox() const;

	UFUNCTION(Server, Reliable)
	void Server_InitializeTeam();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_InitializeTeam();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_PossesNewPawn(APawn* NewPawn);
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	TArray<FNSM_SquadData> Squads;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	TArray<ANSM_CharacterBase*> SquadLeaders;
	
	UFUNCTION(Server, Reliable)
	void Server_PreviewSquadMembers(const TArray<ANSM_CharacterBase*>& SelectedActors);
	
	UFUNCTION(Client, Reliable)
	void Client_PreviewSquadMembers(const TArray<ANSM_CharacterBase*>& SelectedActors);
	
	UFUNCTION(Server, Reliable)
	void Server_UnPreviewSquadMembers(const TArray<ANSM_CharacterBase*>& SelectedActors);
	
	UFUNCTION(Client, Reliable)
	void Client_UnPreviewSquadMembers(const TArray<ANSM_CharacterBase*>& SelectedActors);
	
	UFUNCTION()
	void OnSelected(ANSM_CharacterBase* SelectedActor);
	
	UFUNCTION()
	void OnDeselected(ANSM_CharacterBase* SelectedActor);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_DeselectAllActors();
	
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void Client_DeselectAllActors();

	UFUNCTION()
	void InitializeRTSMode();
	
	UFUNCTION(BlueprintCallable)
	void SelectAllSquadMembers(const TArray<ANSM_CharacterBase*>& SelectedActors);

	UFUNCTION(BlueprintCallable)
	void DeselectAllSquadMembers(TArray<ANSM_CharacterBase*> SelectedActors);
	
	UFUNCTION(BlueprintCallable)
	void CommandSelectedActors(AActor* TargetActor, const UNSM_SquadActionDataAsset* Action);

	UPROPERTY()
	ANSM_BaseHUD* BaseHUD;
	
	UPROPERTY()
	bool bHasInitializedSubSystems;

	UFUNCTION(Server,Reliable)
	void Server_PossessSelectedPawn(ANSM_CharacterBase* CharacterBase);

	virtual void EnableRTSMode();

	virtual void Tick(float DeltaSeconds) override;
	
private:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void Server_EnableRTSMode();

	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(Server, Reliable)
	void Server_PossessPawnAfterCameraTransition(APawn* NewPawn);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PossessPawnAfterCameraTransition(APawn* NewPawn);
	
	FVector GetMousePositionOnTerrain() const;
	
	UFUNCTION()
	void ClearSelection();
	
	UFUNCTION()
	bool HasGroupSelection() const {return ActorsSelected.Num() > 1;}
	
	/** Third person character reference. */
	UPROPERTY(Replicated)
	ANSM_CharacterBase* CharacterRef = nullptr;

	/** Third person character reference. */
	UPROPERTY(Replicated)
	ANSM_RTSPawn* RTSPawnRef = nullptr;

	UPROPERTY(Transient)
	ANSM_SpawnArea* PlayerRespawnArea = nullptr;

	UPROPERTY(Transient, Replicated)
	FGameplayTag PlayerTeamTag;

public:
	ANSM_SpawnArea* GetRespawnArea() const { return PlayerRespawnArea; }
	void SetRespawnArea(ANSM_SpawnArea* SpawnArea) { PlayerRespawnArea = SpawnArea; }

	const FGameplayTag& GetTeamTag() const { return PlayerTeamTag; }
	void SetPlayerTeamTag(const FGameplayTag& TeamTag) { PlayerTeamTag = TeamTag ;}
	
	UFUNCTION(Client, Reliable)
	void Client_InitHUD();

	
	// Time - need to do it here because game state is not owned by the client

	virtual void ReceivedPlayer() override;
	
	UFUNCTION(Server, Reliable)
	void Server_RequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void Client_ReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
};
