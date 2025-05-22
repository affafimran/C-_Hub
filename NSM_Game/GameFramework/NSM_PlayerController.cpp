#include "NSM_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "NSM_BaseHUD.h"
#include "NSM_GameInstance.h"
#include "NSM_GameMode.h"
#include "NSM_GameState.h"
#include "NSM_PlayerStateBase.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameModes/NSM_VersusGameMode.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Character/RTS/NSM_RTSPawn.h"
#include "NordicSimMap/Components/Equipment/NSM_EquipmentComponent.h"
#include "NordicSimMap/Components/Health/NSM_HealthComponent.h"
#include "NordicSimMap/Components/Interaction/NSM_InteractionComponent.h"
#include "NordicSimMap/Subsystem/NSM_CommanderSubsystem.h"


/** Map key to action for mapping context with optional modifiers. */
static void MapKey(UInputMappingContext* InputMappingContext, UInputAction* InputAction, FKey Key, bool bNegate = false, bool bSwizzle = false, EInputAxisSwizzle SwizzleOrder = EInputAxisSwizzle::YXZ)
{
	FEnhancedActionKeyMapping& Mapping = InputMappingContext->MapKey(InputAction, Key);
	UObject* Outer = InputMappingContext->GetOuter();

	if (bNegate)
	{
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Outer);
		Mapping.Modifiers.Add(Negate);
	}

	if (bSwizzle)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Outer);
		Swizzle->Order = SwizzleOrder;
		Mapping.Modifiers.Add(Swizzle);
	}
}

ANSM_PlayerController::ANSM_PlayerController()
{
	FormationManagerComponent = CreateDefaultSubobject<UNSM_FormationManagerComponent>(TEXT("FormationManagerComp"));
	InteractionComponent = CreateDefaultSubobject<UNSM_InteractionComponent>(TEXT("InteractionComponent"));
	BaseHUD = nullptr;
	bHasInitializedSubSystems = false;
	bInRTSMode = false;
	InitialSelectionPoint = FVector2D::ZeroVector;
	CurrentSelectionPoint = FVector2D::ZeroVector;
	CharacterRef = nullptr;
	RTSPawnRef = nullptr;

	bReplicates = true;
	SetReplicateMovement(true);
}

void ANSM_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Create these objects here and not in constructor.
	PawnMappingContext = NewObject<UInputMappingContext>(this);
	
	MoveForwardAction = NewObject<UInputAction>(this);
	MoveForwardAction->ValueType = EInputActionValueType::Axis2D;
	MapKey(PawnMappingContext, MoveForwardAction, EKeys::W);
	MapKey(PawnMappingContext, MoveForwardAction, EKeys::S, true);
	
	MoveRightAction = NewObject<UInputAction>(this);
	MoveRightAction->ValueType = EInputActionValueType::Axis2D;
	MapKey(PawnMappingContext, MoveRightAction, EKeys::A, true);
	MapKey(PawnMappingContext, MoveRightAction, EKeys::D);
	
	MoveUpAction = NewObject<UInputAction>(this);
	MoveUpAction->ValueType = EInputActionValueType::Axis2D;
	MapKey(PawnMappingContext, MoveUpAction, EKeys::Q, true);
	MapKey(PawnMappingContext, MoveUpAction, EKeys::E);
	
	IncrementSpeedAction = NewObject<UInputAction>(this);
	IncrementSpeedAction->ValueType = EInputActionValueType::Axis1D;
	MapKey(PawnMappingContext, IncrementSpeedAction, EKeys::MouseScrollUp);
	MapKey(PawnMappingContext, IncrementSpeedAction, EKeys::MouseScrollDown, true);

	MoveAction = NewObject<UInputAction>(this);
	MoveAction->ValueType = EInputActionValueType::Axis3D;
	MapKey(PawnMappingContext, MoveAction, EKeys::W);
	MapKey(PawnMappingContext, MoveAction, EKeys::S, true);
	MapKey(PawnMappingContext, MoveAction, EKeys::A, true, true);
	MapKey(PawnMappingContext, MoveAction, EKeys::D, false, true);
	
	SwitchWeaponAction = NewObject<UInputAction>(this);
	SwitchWeaponAction->ValueType = EInputActionValueType::Axis1D;
	MapKey(PawnMappingContext, SwitchWeaponAction, EKeys::E);
	MapKey(PawnMappingContext, SwitchWeaponAction, EKeys::Q, true);
	
	SwitchWeaponWheelAction = NewObject<UInputAction>(this);
	SwitchWeaponWheelAction->ValueType = EInputActionValueType::Axis1D;
	MapKey(PawnMappingContext, SwitchWeaponWheelAction, EKeys::MouseScrollUp);
	MapKey(PawnMappingContext, SwitchWeaponWheelAction, EKeys::MouseScrollDown, true);

	CancelAction = NewObject<UInputAction>(this);
	CancelAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, CancelAction, EKeys::Escape);
	
	ZoomAction = NewObject<UInputAction>(this);
	ZoomAction->ValueType = EInputActionValueType::Axis2D;
	MapKey(PawnMappingContext, ZoomAction, EKeys::MouseScrollUp);
	MapKey(PawnMappingContext, ZoomAction, EKeys::MouseScrollDown, true);
	
	VerticalRotateAction = NewObject<UInputAction>(this);
	VerticalRotateAction->ValueType = EInputActionValueType::Axis1D;
	MapKey(PawnMappingContext, VerticalRotateAction, EKeys::MouseY);

	HorizontalRotateAction = NewObject<UInputAction>(this);
	HorizontalRotateAction->ValueType = EInputActionValueType::Axis1D;
	MapKey(PawnMappingContext, HorizontalRotateAction, EKeys::MouseX);

	LeftMouseClick = NewObject<UInputAction>(this);
	LeftMouseClick->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, LeftMouseClick, EKeys::LeftMouseButton);

	RightMouseClick = NewObject<UInputAction>(this);
	RightMouseClick->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, RightMouseClick, EKeys::RightMouseButton);

	LeftAltButton = NewObject<UInputAction>(this);
	LeftAltButton->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, LeftAltButton, EKeys::LeftAlt);

	RightAltButton = NewObject<UInputAction>(this);
	RightAltButton->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, RightAltButton, EKeys::RightAlt);

	BackSpaceButton = NewObject<UInputAction>(this);
	BackSpaceButton->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, BackSpaceButton, EKeys::BackSpace);

	GrenadeAction = NewObject<UInputAction>(this);
	GrenadeAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, GrenadeAction, EKeys::G);

	DropItemAction = NewObject<UInputAction>(this);
	DropItemAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, DropItemAction, EKeys::H);
	
	AimDownSightAction = NewObject<UInputAction>(this);
	AimDownSightAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, AimDownSightAction, EKeys::RightMouseButton);
	
	SprintAction = NewObject<UInputAction>(this);
	SprintAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, SprintAction, EKeys::LeftShift);
	
	CrouchAction = NewObject<UInputAction>(this);
	CrouchAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, CrouchAction, EKeys::LeftControl);

	StopFormationAction = NewObject<UInputAction>(this);
	StopFormationAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, StopFormationAction, EKeys::RightAlt);

	ChangeFormationAction = NewObject<UInputAction>(this);
	ChangeFormationAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, ChangeFormationAction, EKeys::LeftControl);

	SplitFormationAction = NewObject<UInputAction>(this);
	SplitFormationAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, SplitFormationAction, EKeys::L);
	
	RTSCameraSteppingModeAction = NewObject<UInputAction>(this);
	RTSCameraSteppingModeAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, RTSCameraSteppingModeAction, EKeys::LeftShift);
	
	PossessSelectedPawn = NewObject<UInputAction>(this);
	PossessSelectedPawn->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, PossessSelectedPawn, EKeys::F);
	
	RevertSquadOrdersAction = NewObject<UInputAction>(this);
	RevertSquadOrdersAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, RevertSquadOrdersAction, EKeys::G);
	
	MoveToOrderAction = NewObject<UInputAction>(this);
	MoveToOrderAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, MoveToOrderAction, EKeys::RightMouseButton);
	
	SquadOneAction = NewObject<UInputAction>(this);
	SquadOneAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, SquadOneAction, EKeys::One);
	
	SquadTwoAction = NewObject<UInputAction>(this);
	SquadTwoAction->ValueType = EInputActionValueType::Boolean;
	MapKey(PawnMappingContext, SquadTwoAction, EKeys::Two);
	
	SquadThreeAction = NewObject<UInputAction>(this);
	SquadThreeAction->ValueType = EInputActionValueType::Boolean;	
	MapKey(PawnMappingContext, SquadThreeAction, EKeys::Three);
	
	SquadFourAction = NewObject<UInputAction>(this);
	SquadFourAction->ValueType = EInputActionValueType::Boolean;	
	MapKey(PawnMappingContext, SquadFourAction, EKeys::Four);
	
	SquadFiveAction = NewObject<UInputAction>(this);
	SquadFiveAction->ValueType = EInputActionValueType::Boolean;	
	MapKey(PawnMappingContext, SquadFiveAction, EKeys::Five);
	
	SquadSixAction = NewObject<UInputAction>(this);
	SquadSixAction->ValueType = EInputActionValueType::Boolean;	
	MapKey(PawnMappingContext, SquadSixAction, EKeys::Six);
	
	SquadSevenAction = NewObject<UInputAction>(this);
	SquadSevenAction->ValueType = EInputActionValueType::Boolean;	
	MapKey(PawnMappingContext, SquadSevenAction, EKeys::Seven);
	
	if (InputComponent)
	{
		InputComponent->BindAction("SwitchMode", IE_Pressed, this, &ANSM_PlayerController::EnableRTSMode);
	}
}

void ANSM_PlayerController::OnRep_InRTSMode()
{
	if (bInRTSMode)
	{		
		SetShowMouseCursor(true);
		bEnableMouseOverEvents = true;
		InRTSModeEvent.Broadcast(true);
	}
	else
	{		
		SetShowMouseCursor(false);
		bEnableMouseOverEvents = false;
		InRTSModeEvent.Broadcast(false);
	}
}

void ANSM_PlayerController::OpenRadialWheel()
{
	FVector2D MousePosition;
	GetMousePosition(MousePosition.X, MousePosition.Y);
	
	TArray<UNSM_SquadActionDataAsset*> NewActions;
	if(IsValid(InteractionComponent->FocusedActor))
	{
		const UNSM_TeamComponent* FocusedActorTeamComponent = InteractionComponent->FocusedActor->FindComponentByClass<UNSM_TeamComponent>();
		const UNSM_TeamComponent* CharacterTeamComponent = CharacterRef->FindComponentByClass<UNSM_TeamComponent>();
		const UNSM_ActionComponent* FocusedActorActionComponent = InteractionComponent->FocusedActor->FindComponentByClass<UNSM_ActionComponent>();

		InteractionComponent->LastFocusedActor = InteractionComponent->FocusedActor;
		if(!IsValid(FocusedActorActionComponent))
		{
			NewActions = GeneralSquadActions;
		}
		else if(IsValid(FocusedActorTeamComponent))
		{
			if(IsValid(CharacterTeamComponent))
			{
				if(FocusedActorTeamComponent->GetTeamTag() == CharacterTeamComponent->GetTeamTag())
				{
					NewActions = FocusedActorActionComponent->FriendlyActions;
				}
				else
				{
					NewActions = FocusedActorActionComponent->InteractableActions;
				}
			}
			else
			{
				NewActions = FocusedActorActionComponent->InteractableActions;
			}
		}
		else
		{
			NewActions = FocusedActorActionComponent->InteractableActions;
		}
	}
	else
	{
		NewActions = GeneralSquadActions;
	}

	if(!IsValid(BaseHUD))
	{
		return;
	}
	if (BaseHUD->RadialMenuWheel->GetClass()->ImplementsInterface(UNSM_RadialWheelInterface::StaticClass()))
	{
		int32 ScreenWidth, ScreenHeight;
		GetViewportSize(ScreenWidth, ScreenHeight);
		const FVector2D ScreenSize(static_cast<float>(ScreenWidth), static_cast<float>(ScreenHeight));
		const FVector2D MousePositionNormalized = MousePosition / ScreenSize;

		const FVector2D WidgetSize = BaseHUD->RadialMenuWheel->GetDesiredSize();
		const FVector2D CorrectedMousePosition = MousePositionNormalized * ScreenSize - WidgetSize / 2.0f;
        
		BaseHUD->RadialMenuWheel->SetPositionInViewport(CorrectedMousePosition, false);
        
		
		BaseHUD->RadialMenuWheel->SetVisibility(ESlateVisibility::Visible);
		BaseHUD->RadialMenuWheel->WheelActions = NewActions;
		BaseHUD->RadialMenuWheel->bUseTick = true;
		if(bInRTSMode)
		{
			SetShowMouseCursor(false);
		}
		INSM_RadialWheelInterface::Execute_UpdateWheelActions(BaseHUD->RadialMenuWheel);
	}
}

void ANSM_PlayerController::Server_SetCharacterReference_Implementation(ANSM_CharacterBase* InCharacterRef)
{
	SetCharacterReference(InCharacterRef);
}

void ANSM_PlayerController::CloseRadialWheel()
{
	if(HasAuthority() && IsValid(BaseHUD))
	{
		BaseHUD->RadialMenuWheel->SetVisibility(ESlateVisibility::Collapsed);
		BaseHUD->RadialMenuWheel->bUseTick = false;
		InteractionComponent->LastFocusedActor = nullptr;
		if(bInRTSMode)
		{
			SetShowMouseCursor(true);
		}
	}
}

void ANSM_PlayerController::InitializePlayer(const int32 PlayerAmount)
{
	if (bHasInitializedSubSystems)
	{
		return;
	}
	bHasInitializedSubSystems = true;
	
	if (ANSM_PlayerStateBase* PlayerStateBase = GetPlayerState<ANSM_PlayerStateBase>())
	{
		PlayerStateBase->AssignTeam(PlayerAmount);
		//PlayerStateBase->InitializeSystem();
	}
}

void ANSM_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	BaseHUD = Cast<ANSM_BaseHUD>(GetHUD());
	if(!IsValid(BaseHUD))
	{
		return;
	}
	
	BaseHUD->SetHUD();
}

void ANSM_PlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSM_PlayerController, RTSWidgetClass);
	DOREPLIFETIME(ANSM_PlayerController, bInRTSMode);
	DOREPLIFETIME(ANSM_PlayerController, SquadLeaders);
	DOREPLIFETIME(ANSM_PlayerController, ActorsSelected);
	DOREPLIFETIME(ANSM_PlayerController, GeneralSquadActions);
	DOREPLIFETIME(ANSM_PlayerController, CharacterRef);
	DOREPLIFETIME(ANSM_PlayerController, RTSPawnRef);
	DOREPLIFETIME(ANSM_PlayerController, Squads);
	DOREPLIFETIME(ANSM_PlayerController, PlayerTeamTag);
}


void ANSM_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	Server_DeselectAllActors();
	ANSM_CharacterBase* CharacterBase = Cast<ANSM_CharacterBase>(InPawn);
	if(IsValid(CharacterBase) && CharacterBase != CharacterRef)
	{
		CharacterRef = CharacterBase;
		SetShowMouseCursor(false);
		bEnableMouseOverEvents = false;
		if(IsValid(RTSPawnRef))
		{
			RTSPawnRef->Server_SetIsRTSInitialized(false);
		}

		bInRTSMode = false;
		
		Server_DeselectAllActors();
		InteractionComponent->Server_SetIsInRTSMode(false);
		InRTSModeEvent.Broadcast(false);
	}
	
	const FInputModeGameOnly GameModeOnly;
	SetInputMode(GameModeOnly);
}

void ANSM_PlayerController::Server_PossessSelectedPawn_Implementation(ANSM_CharacterBase* CharacterBase)
{
	if (IsValid(CharacterBase))
	{		
		Server_DeselectAllActors();
		SetCharacterReference(CharacterBase);
		EnableRTSMode();
	}
}

void ANSM_PlayerController::Server_PossessPawnAfterCameraTransition_Implementation(APawn* NewPawn)
{
	Multicast_PossessPawnAfterCameraTransition(NewPawn);
}

void ANSM_PlayerController::CommandSelectedActors(AActor* TargetActor, const UNSM_SquadActionDataAsset* Action)
{
	if (ActorsSelected.Num() <= 0)
	{
		return;
	}
	
	if(!IsValid(Action))
	{
		return;
	}

	for (const AActor* ActorSelected : ActorsSelected)
	{
		if(IsValid(ActorSelected))
		{
			UNSM_CommandComponent* CommandComponent = ActorSelected->FindComponentByClass<UNSM_CommandComponent>();
			if(IsValid(CommandComponent))
			{
				CommandComponent->CommandData.GoalLocation = TargetActor->GetActorLocation();
				CommandComponent->CommandData.Target = TargetActor;
				CommandComponent->CommandData.CommandObject = Action->CommandObject;
				CommandComponent->CommandType = ENSM_Command::Command;
				const UNSM_CommandBase* CommandBase = NewObject<UNSM_CommandBase>(GetPawn(), Action->CommandObject);
				CommandComponent->GeneralCommandDelegate.Broadcast(CommandBase, CommandComponent->CommandType);
			}
		}
	}
}

void ANSM_PlayerController::EnableRTSMode()
{
	Server_EnableRTSMode();	
}

void ANSM_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);	

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{				
		if (UNSM_CommanderSubsystem* Command = LocalPlayer->GetSubsystem<UNSM_CommanderSubsystem>())
		{
			Command->Update(this);
		}
	}
}

void ANSM_PlayerController::Server_EnableRTSMode_Implementation()
{
	if(!IsValid(RTSPawnRef) || !IsValid(CharacterRef) )
	{
		return;
	}

	APawn* TargetActor;
	if(bInRTSMode)
	{
		if (CharacterRef->bIsDead)
		{
			return;
		}
		
		TargetActor = CharacterRef;
		RTSPawnRef->Server_SetIsRTSInitialized(false);
		InteractionComponent->Server_SetIsInRTSMode(false);
		CharacterRef->Multicast_SetCommanderWidgetVisible(false);
	}
	else
	{
		TargetActor = RTSPawnRef;
		if(!RTSPawnRef->IsRTSInitialized())
		{
			RTSPawnRef->Server_SetIsRTSInitialized(true);
		}
		if(IsValid(CharacterRef->GetAIController()))
		{
			CharacterRef->GetAIController()->Server_PossesNewPawn(CharacterRef);
		}
		
		InteractionComponent->Server_SetIsInRTSMode(true);
		CharacterRef->Multicast_SetCommanderWidgetVisible(true);

		if (IsLocalPlayerController())
		{
			const ULocalPlayer* LocalPlayer = GetLocalPlayer();
			if (LocalPlayer && LocalPlayer->ViewportClient)
			{
				if(FViewport* Viewport = LocalPlayer->ViewportClient->Viewport)
				{
					FVector2D ViewportSize;
					LocalPlayer->ViewportClient->GetViewportSize(ViewportSize);
					const int32 X = static_cast<int32>(ViewportSize.X * 0.5f);
					const int32 Y = static_cast<int32>(ViewportSize.Y * 0.5f);

					Viewport->SetMouse(X, Y);
				}
			}
		}

		//[Vlad] - Will do reset order from the RTS pawn
		// if (GetWorld())
		// {			
		// 	if(ANSM_VersusGameMode* GameMode = GetWorld()->GetAuthGameMode<ANSM_VersusGameMode>())
		// 	{
		// 		GameMode->AuthOnSwitchToRTSMode(CharacterRef);
		// 	}
		// }
	}

	Server_DeselectAllActors();
	bInRTSMode = !bInRTSMode;
	OnRep_InRTSMode();
	
	if (!IsValid(TargetActor))
	{
		return;
	}

	Server_PossesNewPawn(TargetActor);
	return;
	
	constexpr float BlendTime = 0.25f;
	SetViewTargetWithBlend(TargetActor, BlendTime, VTBlend_Linear, BlendTime, false);

	DisableInput(this);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ANSM_PlayerController::Server_PossessPawnAfterCameraTransition, TargetActor);
	GetWorldTimerManager().SetTimer(CameraTransitionTimerHandle, TimerDelegate, BlendTime, false);
}

void ANSM_PlayerController::Server_PreviewSquadMembers_Implementation(const TArray<ANSM_CharacterBase*>& SelectedActors)
{
	Client_PreviewSquadMembers(SelectedActors);
}

void ANSM_PlayerController::Client_PreviewSquadMembers_Implementation(const TArray<ANSM_CharacterBase*>& SelectedActors)
{
	for (const AActor* Actor : SelectedActors)
	{
		if(!IsValid(Actor))
		{
			continue;
		}
		const UNSM_HealthComponent* HealthComponent = Actor->FindComponentByClass<UNSM_HealthComponent>();
		if(!IsValid(HealthComponent) && !HealthComponent->IsAlive())
		{
			continue;
		}
		const UNSM_RTSActorSelectionComponent* ActorSelectionComponent = Actor->FindComponentByClass<UNSM_RTSActorSelectionComponent>();
		if(!IsValid(ActorSelectionComponent))
		{
			continue;
		}
	
		if (UNiagaraComponent* SelectedActorDecalComponent = Actor->FindComponentByClass<UNiagaraComponent>())
		{
			SelectedActorDecalComponent->Activate();
		}
	}
}

void ANSM_PlayerController::Server_UnPreviewSquadMembers_Implementation(
	const TArray<ANSM_CharacterBase*>& SelectedActors)
{
	Client_UnPreviewSquadMembers(SelectedActors);
}

void ANSM_PlayerController::Client_UnPreviewSquadMembers_Implementation(
	const TArray<ANSM_CharacterBase*>& SelectedActors)
{
	for (const AActor* Actor : SelectedActors)
	{
		const UNSM_HealthComponent* HealthComponent = Actor ? Actor->FindComponentByClass<UNSM_HealthComponent>() : nullptr;
		if(!IsValid(HealthComponent) && !HealthComponent->IsAlive())
		{
			continue;
		}
		const UNSM_RTSActorSelectionComponent* ActorSelectionComponent = Actor->FindComponentByClass<UNSM_RTSActorSelectionComponent>();
		if(!IsValid(ActorSelectionComponent))
		{
			return;
		}
	
		if (UNiagaraComponent* SelectedActorDecalComponent = Actor->FindComponentByClass<UNiagaraComponent>())
		{
			SelectedActorDecalComponent->DeactivateImmediate();
		}
	}
}

void ANSM_PlayerController::OnSelected(ANSM_CharacterBase* SelectedActor)
{
	if (!IsValid(SelectedActor))
	{
		return;
	}
	
	const UNSM_HealthComponent* HealthComponent = SelectedActor->FindComponentByClass<UNSM_HealthComponent>();
	if(!IsValid(HealthComponent) && !HealthComponent->IsAlive())
	{
		return;
	}

	if(ActorsSelected.Contains(SelectedActor))
	{
		return;
	}
	
	UNSM_RTSActorSelectionComponent* ActorSelectionComponent = SelectedActor->FindComponentByClass<UNSM_RTSActorSelectionComponent>();
	if(!IsValid(ActorSelectionComponent))
	{
		return;
	}

	if (UNiagaraComponent* SelectedActorDecalComponent = SelectedActor->FindComponentByClass<UNiagaraComponent>())
	{
		SelectedActorDecalComponent->Activate();
	}
	ActorsSelected.Add(SelectedActor);
	ActorSelectionComponent->IsCharacterSelectedEvent.Broadcast(true);
	ActorSelectionComponent->bIsSelected = true;
}

void ANSM_PlayerController::Client_DeselectAllActors_Implementation()
{
	TArray<ANSM_CharacterBase*> TempArray = ActorsSelected;
	for (ANSM_CharacterBase* Actor : TempArray)
	{
		if (IsValid(Actor))
		{
			OnDeselected(Actor);
		}
	}
}

void ANSM_PlayerController::InitializeRTSMode()
{
	if(!IsValid(GetPawn()))
	{
		return;
	}

	CharacterRef = Cast<ANSM_CharacterBase>(GetPawn());
	
	if(!IsValid(CharacterRef))
	{
		return;
	}

	if(!IsValid(RTSPawnRef))
	{
		RTSPawnRef = CharacterRef->RTSPawnRef;
	}
}

void ANSM_PlayerController::SelectAllSquadMembers(const TArray<ANSM_CharacterBase*>& SelectedActors)
{
	if(SelectedActors.Num() <= 0)
	{
		return;
	}
	
	for (ANSM_CharacterBase* SelectedActor : SelectedActors)
	{
		if(!ActorsSelected.Contains(SelectedActor))
		{
			OnSelected(SelectedActor);
		}
	}
}

void ANSM_PlayerController::DeselectAllSquadMembers(TArray<ANSM_CharacterBase*> SelectedActors)
{
	if(SelectedActors.Num() <= 0)
	{
		return;
	}
	
	for (ANSM_CharacterBase* SelectedActor : SelectedActors)
	{
		if(ActorsSelected.Contains(SelectedActor))
		{
			if (IsValid(SelectedActor))
			{
				OnDeselected(SelectedActor);
			}
		}
	}
}

void ANSM_PlayerController::UpdateSelectionBox() const
{
	if (BaseHUD)
	{
		BaseHUD->InitialSelectionPoint = BaseHUD->GetMousePositionForSelection();
		BaseHUD->bStartSelection = true;
	}
}

void ANSM_PlayerController::Server_PossesNewPawn_Implementation(APawn* NewPawn)
{
	if (ANSM_CharacterBase* CharacterBase = Cast<ANSM_CharacterBase>(NewPawn))
	{		
		SetCharacterReference(CharacterBase);
	}
	
	Possess(NewPawn);
}

void ANSM_PlayerController::Multicast_InitializeTeam_Implementation()
{
	const ANSM_PlayerStateBase* LocalPlayerState = GetPlayerState<ANSM_PlayerStateBase>();
	if(!IsValid(LocalPlayerState))
	{
		return;
	}
	
	for (FNSM_SquadData Squad : Squads)
	{
		for (ANSM_CharacterBase* Member : Squad.SquadMembers)
		{
			Member->FormationUnitComponent->Server_SetCurrentSquadData(Squad);
			if(!IsValid(Member))
			{
				continue;
			}
			if(SquadLeaders.Contains(Member))
			{
				Member->Server_SetIsSquadLeader(true);
			}
			Member->TeamComponent->AuthSetTeamTag(LocalPlayerState->PlayerTeamTag);
			Member->OnTeamAssigned(true);
		}
	}
}

void ANSM_PlayerController::Server_InitializeTeam_Implementation()
{
	Multicast_InitializeTeam();
}

void ANSM_PlayerController::EndSelectionBox() const
{
	if (BaseHUD)
	{
		BaseHUD->InitialSelectionPoint = FVector2D(0.f,0.f);
		BaseHUD->bStartSelection = false;
		BaseHUD->ActorsSelected.Empty();
	}
}

void ANSM_PlayerController::OnDeselected(ANSM_CharacterBase* SelectedActor)
{
	if (!IsValid(SelectedActor))
	{
		return;
	}

	UNSM_RTSActorSelectionComponent* ActorSelectionComponent = SelectedActor->FindComponentByClass<UNSM_RTSActorSelectionComponent>();
	if(!IsValid(ActorSelectionComponent))
	{
		return;
	}
	
	if (UNiagaraComponent* SelectedActorDecalComponent = SelectedActor->FindComponentByClass<UNiagaraComponent>())
	{
		SelectedActorDecalComponent->DeactivateImmediate();
	}
	ActorsSelected.Remove(SelectedActor);
	ActorSelectionComponent->IsCharacterSelectedEvent.Broadcast(false);
	ActorSelectionComponent->bIsSelected = false;
}

void ANSM_PlayerController::Server_DeselectAllActors_Implementation()
{
	Client_DeselectAllActors();
}

void ANSM_PlayerController::Multicast_PossessPawnAfterCameraTransition_Implementation(APawn* NewPawn)
{
	if(NewPawn)
	{
		Possess(NewPawn);
		if(GetPawn()->GetMovementComponent())
		{
			GetPawn()->GetMovementComponent()->SetActive(true);
		}
	}
	EnableInput(this);
}

FVector ANSM_PlayerController::GetMousePositionOnTerrain() const
{
	FHitResult Hit;
	if(GetHitResultUnderCursor(ECC_Visibility, true,Hit))
	{
		if(Hit.bBlockingHit)
		{
			return Hit.ImpactPoint;
		}
	}
	
	return FVector::ZeroVector;
}

void ANSM_PlayerController::ClearSelection()
{
	TArray<ANSM_CharacterBase*> TemporalArray = ActorsSelected;
	for (ANSM_CharacterBase* ActorSelected : TemporalArray)
	{
		if(IsValid(ActorSelected))
		{
			OnDeselected(ActorSelected);
		}
	}
	ActorsSelected.Empty();
}

void ANSM_PlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalPlayerController())
	{
		Server_RequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ANSM_PlayerController::Client_ReportServerTime_Implementation(float TimeOfClientRequest,
                                                                   float TimeServerReceivedClientRequest)
{	
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	const float CurrentServerTime = TimeServerReceivedClientRequest + 0.5f * RoundTripTime;


	if (ANSM_GameState* ANSMGameState = GetWorld()->GetGameState<ANSM_GameState>())
	{		
		ANSMGameState->SetClientServerDeltaTime(CurrentServerTime - GetWorld()->GetTimeSeconds());
	}
}

void ANSM_PlayerController::Server_RequestServerTime_Implementation(float TimeOfClientRequest)
{	
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	Client_ReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ANSM_PlayerController::Client_InitHUD_Implementation()
{
	if (BaseHUD)
	{
		if (BaseHUD->UserWidget)
		{
			BaseHUD->UserWidget->InitializeWidget(this);
		}		
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}
