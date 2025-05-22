#include "NSM_BaseHUD.h"
#include "NSM_GameState.h"
#include "NSM_PlayerController.h"
#include "NordicSimMap/UI/NSM_BaseWidget.h"
#include "NordicSimMap/UI/RadialWheel/NSM_RadialMenuWheelBase.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"
#include "NordicSimMap/UI/NSM_EndGameScreen.h"

class ANSM_GameState;

void FHUDPackage::Reset()
{	
	CrosshairCenter = nullptr;
	CrosshairLeft = nullptr;
	CrosshairRight = nullptr;
	CrosshairTop = nullptr;
	CrosshairBottom = nullptr;
	CrosshairSpread = 0.f;
	BaseCrosshairSpread = 0.f;
	BaseHitmarkerSpread = 10.f;
	HitmarkerSpread = -1.f;
	HitmarkerBottomRight = nullptr;
	HitmarkerBottomLeft = nullptr;
	HitmarkerTopLeft= nullptr;
	HitmarkerTopRight = nullptr;
}

void ANSM_BaseHUD::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsValid(GetWorld()))
	{
		return;
	}

	if (APlayerController* FirstPlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController = Cast<ANSM_PlayerController>(FirstPlayerController);
	}
	bStartSelection = false;
}

void ANSM_BaseHUD::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSM_BaseHUD, PlayerController);
	DOREPLIFETIME(ANSM_BaseHUD, ActorsSelected);
	DOREPLIFETIME(ANSM_BaseHUD, InitialSelectionPoint);
	DOREPLIFETIME(ANSM_BaseHUD, CurrentSelectionPoint);
}

void ANSM_BaseHUD::SetHUD()
{
	if(UserWidgetClass != nullptr)
	{
		UserWidget = CreateWidget<UNSM_HUDWidget>(GetOwningPlayerController(), UserWidgetClass);
		if(IsValid(UserWidget))
		{
			UserWidget->AddToViewport(1);
			UserWidget->InitializeWidget(GetOwningPlayerController());
			UserWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
	if(RadialMenuWheelClass != nullptr)
	{
		RadialMenuWheel = CreateWidget<UNSM_RadialMenuWheelBase>(GetOwningPlayerController(),RadialMenuWheelClass);
		if(IsValid(RadialMenuWheel))
		{
			const FVector2D CursorSize(90.0f, 90.0f);
			const FVector2D WidgetSize = CursorSize * 2.0f;
			RadialMenuWheel->AddToViewport(0);
			RadialMenuWheel->InitializeWidget(GetOwningPlayerController());
			RadialMenuWheel->SetDesiredSizeInViewport(WidgetSize);
			RadialMenuWheel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

FVector2d ANSM_BaseHUD::GetMousePositionForSelection() const
{
	//Get the mouse position of the owning  PlayerController to draw the selection
	float PosY = 0.f;
	float PosX = 0.f;
	if (GetOwningPlayerController())
	{
		GetOwningPlayerController()->GetMousePosition(PosX,PosY);
	}
	return FVector2D(PosX, PosY);
}

void ANSM_BaseHUD::ShouldSelectValidActors() const
{
	if(!IsValid(PlayerController) || PlayerController->ActorsSelected.IsEmpty())
	{
		return;
	}

	TArray<ANSM_CharacterBase*> CharactersSelected = PlayerController->ActorsSelected;
	for (ANSM_CharacterBase* SelectedActor : CharactersSelected)
	{
		if (IsValid(SelectedActor) && SelectedActor->FindComponentByClass<UNSM_RTSActorSelectionComponent>())
		{
			if (!ActorsSelected.Contains(SelectedActor))
			{
				PlayerController->OnDeselected(SelectedActor);
			}
		}
	}
}

void ANSM_BaseHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!IsValid(PlayerController))
	{
		return;
	}
	
	if (GEngine && !PlayerController->bInRTSMode)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D& ViewportCenter (ViewportSize * 0.5f);

		const float SpreadScaled = HUDPackage.BaseCrosshairSpread * HUDPackage.CrosshairSpread;

		DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, FVector2D::ZeroVector, HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, FVector2D(-SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, FVector2D(SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, FVector2D(0.f, -SpreadScaled), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, FVector2D(0.f, SpreadScaled), HUDPackage.CrosshairsColor);

		if (HUDPackage.HitmarkerSpread > -UE_KINDA_SMALL_NUMBER &&
		    HUDPackage.HitmarkerSpread < 1.f)
		{
			const float HitMarkerSpreadScaled = HUDPackage.BaseHitmarkerSpread * HUDPackage.HitmarkerSpread;
			
			DrawCrosshair(HUDPackage.HitmarkerTopLeft, ViewportCenter, FVector2D(-HitMarkerSpreadScaled, -HitMarkerSpreadScaled), HUDPackage.HitmarkerColor, 1.f);
			DrawCrosshair(HUDPackage.HitmarkerBottomLeft, ViewportCenter, FVector2D(-HitMarkerSpreadScaled, HitMarkerSpreadScaled), HUDPackage.HitmarkerColor, 1.f);
			DrawCrosshair(HUDPackage.HitmarkerTopRight, ViewportCenter, FVector2D(HitMarkerSpreadScaled, -HitMarkerSpreadScaled), HUDPackage.HitmarkerColor, 1.f);
			DrawCrosshair(HUDPackage.HitmarkerBottomRight, ViewportCenter, FVector2D(HitMarkerSpreadScaled, HitMarkerSpreadScaled), HUDPackage.HitmarkerColor, 1.f);		
		}
		else
		{
			HUDPackage.HitmarkerSpread = -1.f;
		}
	}

	if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
	{		
		if (const UNSM_CommanderSubsystem* Command = LocalPlayer->GetSubsystem<UNSM_CommanderSubsystem>())
		{
			UserWidget->UpdateLocalPlayerLeaders(Command->GetLeaderData());
		}
	}
	
	if(!bStartSelection || 
	   !IsValid(PlayerController->GetPawn()))
	{
		return;
	}
	
	CurrentSelectionPoint = GetMousePositionForSelection();

	constexpr FLinearColor RectColor = FLinearColor(0, .5f, 1, .15f);
	DrawRect(RectColor, InitialSelectionPoint.X, InitialSelectionPoint.Y,
	(CurrentSelectionPoint.X - InitialSelectionPoint.X),
	(CurrentSelectionPoint.Y - InitialSelectionPoint.Y));
	
	ActorsSelected.Empty();
	GetActorsInSelectionRectangle<ANSM_CharacterBase>(InitialSelectionPoint,
	CurrentSelectionPoint,ActorsSelected, false, false);

	if(ActorsSelected.IsEmpty())
	{
		ShouldSelectValidActors();
		return;
	}

	TArray<ANSM_CharacterBase*> Actors = ActorsSelected;
	for (const ANSM_CharacterBase* ActorInSelection : Actors)
	{
		if (!IsValid(ActorInSelection))
		{
			continue;
		}
		
		if(!IsValid(ActorInSelection->TeamComponent))
		{
			continue;
		}

		if(!ActorInSelection->TeamComponent->GetTeamTag().MatchesTagExact(PlayerController->GetTeamTag()))
		{
			continue;
		}
		if (!IsValid(ActorInSelection->RTSActorSelectionComponent))
		{
			continue;
		}

		TArray<ANSM_CharacterBase*> Members;
		for (const FNSM_SquadData Squad: PlayerController->Squads)
		{
			if(Squad.SquadMembers.Contains(ActorInSelection))
			{
				Members = Squad.SquadMembers;
				break;
			}
		}
		if(Members.IsEmpty())
		{
			continue;
		}
		PlayerController->SelectAllSquadMembers(Members);
		ActorsSelected.Append(Members);
	}
	
	ShouldSelectValidActors();
}

void ANSM_BaseHUD::SetHUDTimerValue(float Value) const
{
	if (UserWidget)
	{
		UserWidget->SetMatchTimer(Value);
	}
}

void ANSM_BaseHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread,
                                 FLinearColor CrosshairColor, float Scale/* = 1.f*/)
{
	if (!Texture)
	{
		return;
	}
	
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - TextureWidth * 0.5f + Spread.X,
		ViewportCenter.Y - TextureHeight * 0.5f + Spread.Y
		);

	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairColor,
		BLEND_Translucent,
		Scale);
}

UNSM_EndGameScreen* ANSM_BaseHUD::GetEndGameScreen()
{
	if (!EndGameScreenWidget)
	{
		if(EndGameScreen != nullptr)
		{
			EndGameScreenWidget = CreateWidget<UNSM_EndGameScreen>(GetOwningPlayerController(), EndGameScreen);
			
			if(IsValid(EndGameScreenWidget))
			{
				EndGameScreenWidget->AddToViewport(1000);
				EndGameScreenWidget->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}

	return EndGameScreenWidget;
}
