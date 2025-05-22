#include "NSM_GameState.h"
#include "NSM_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NordicSimMap/Core/NSM_GeneralStatics.h"
#include "NordicSimMap/LevelIngredients/NSM_LevelData.h"
#include "NordicSimMap/UI/NSM_EndGameScreen.h"


ANSM_GameState::ANSM_GameState()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
}

void ANSM_GameState::BeginPlay()
{
	Super::BeginPlay();
	
	if (const UNSM_LevelData* LevelData = UNSM_GeneralStatics::GetLevelData(GetWorld()))
	{
		TimeRemaining = LevelData->GetMatchDuration();
	}
}

void ANSM_GameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const uint32 SecondsLeft = FMath::CeilToInt(FMath::Max(0.f,TimeRemaining - GetServerTime()));

	if (CountdownInt != SecondsLeft)
	{		
		CountdownInt = SecondsLeft;

		if (ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
		{
			if (PlayerController->BaseHUD)
			{
				PlayerController->BaseHUD->SetHUDTimerValue(TimeRemaining - GetServerTime());

				if (SecondsLeft <= 0u)
				{
					const FInputModeUIOnly UIOnly;
					PlayerController->SetInputMode(UIOnly);
					if (const UNSM_EndGameScreen* EndGameScreen = PlayerController->BaseHUD->GetEndGameScreen())
					{
						EndGameScreen->SetEndGameResult(FGameplayTag::EmptyTag, PlayerController->GetTeamTag(), EEndGameReason::TimeExpired);
					}
				}
			}
		}
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		TimeSyncRunningTime += DeltaTime;
		if (TimeSyncRunningTime > TimeSyncFrequency)
		{
			if (ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
			{
				PlayerController->Server_RequestServerTime(GetWorld()->GetTimeSeconds());
			}		
			
			TimeSyncRunningTime = 0.f;
		}
	}
}

void ANSM_GameState::InitializePlayerTeam()
{
	for (const APlayerState* Player : PlayerArray)
	{
		if(!IsValid(Player))
		{
			continue;
		}
		ANSM_PlayerController* PlayerController = static_cast<ANSM_PlayerController*>(Player->GetPlayerController());
		if(!IsValid(PlayerController))
		{
			continue;
		}
		PlayerController->Server_InitializeTeam();
	}
}

float ANSM_GameState::GetServerTime() const
{
	if (HasAuthority())
	{
		return  GetWorld()->GetTimeSeconds();
	}
	
	return GetWorld()->GetTimeSeconds() + ClientServerDeltaTime;
}

void ANSM_GameState::Multicast_EndGameObjectiveDestroyed_Implementation(const FGameplayTag& DestroyedTag)
{
	if (ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		if (PlayerController->BaseHUD && PlayerController->BaseHUD->GetEndGameScreen())
		{
			if (const UNSM_EndGameScreen* EndGameScreen = PlayerController->BaseHUD->GetEndGameScreen())
			{				
				FGameplayTag WinningTeam = FGameplayTag::EmptyTag;
				if (DestroyedTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Team.One"))))
				{
					WinningTeam = FGameplayTag::RequestGameplayTag(FName("Team.Two"));
				}
				else if (DestroyedTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Team.Two"))))
				{			
					WinningTeam = FGameplayTag::RequestGameplayTag(FName("Team.One"));
				}

				const FInputModeUIOnly UIOnly;
				PlayerController->SetInputMode(UIOnly);
				EndGameScreen->SetEndGameResult(WinningTeam, PlayerController->GetTeamTag(), EEndGameReason::ObjectiveDestroyed);
			}
		}		
	}
}

