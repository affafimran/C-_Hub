#include "NSM_GameMode.h"
#include "AnimationBudgetAllocatorParameters.h"
#include "IAnimationBudgetAllocator.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NSM_PlayerController.h"

ANSM_GameMode::ANSM_GameMode()
{
	bAllExistingPlayersRegistered = false;
}

void ANSM_GameMode::BeginPlay()
{
	Super::BeginPlay();	
	
	if (IAnimationBudgetAllocator* Interface = IAnimationBudgetAllocator::Get(GetWorld()))
	{
		FAnimationBudgetAllocatorParameters Params;
		Params.AutoCalculatedSignificanceMaxDistance = 10000.f;
		Params.AutoCalculatedSignificanceMaxDistanceSqr = FMath::Square(10000.f);
		
		Interface->SetParameters(Params);
		Interface->SetEnabled(true);
	}
}

void ANSM_GameMode::PostLogin(APlayerController* NewPlayer)
{
	if (!this->bAllExistingPlayersRegistered)
	{
		// RegisterExistingPlayers has not run yet. When it does, it will register this incoming player
		// controller.
		Super::PostLogin(NewPlayer);
		return;
	}

	check(IsValid(NewPlayer));

	// This code handles logins for both the local player (listen server) and remote players (net connection).
	FUniqueNetIdRepl UniqueNetIdRepl;
	if (NewPlayer->IsLocalPlayerController())
	{
		ULocalPlayer *LocalPlayer = NewPlayer->GetLocalPlayer();
		if (IsValid(LocalPlayer))
		{
			UniqueNetIdRepl = LocalPlayer->GetPreferredUniqueNetId();
		}
		else
		{
			UNetConnection *RemoteNetConnection = Cast<UNetConnection>(NewPlayer->Player);
			check(IsValid(RemoteNetConnection));
			UniqueNetIdRepl = RemoteNetConnection->PlayerId;
		}
	}
	else
	{
		UNetConnection *RemoteNetConnection = Cast<UNetConnection>(NewPlayer->Player);
		check(IsValid(RemoteNetConnection));
		UniqueNetIdRepl = RemoteNetConnection->PlayerId;
	}

	// Get the unique player ID.
	TSharedPtr<const FUniqueNetId> UniqueNetId = UniqueNetIdRepl.GetUniqueNetId();
	check(UniqueNetId != nullptr);

	// Get the online session interface.
	IOnlineSubsystem *Subsystem = Online::GetSubsystem(NewPlayer->GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	// Register the player with the "MyLocalSessionName" session; this name should match the name you provided in CreateSession.
	if (!Session->RegisterPlayer(CustomSessionName, *UniqueNetId, false))
	{
		// The player could not be registered; typically you will want to kick the player from the server in this situation.
	}

	Super::PostLogin(NewPlayer);
}

void ANSM_GameMode::PlayerEliminated(ANSM_CharacterBase* EliminatedCharacter, AController* VictimController,
	AController* AttackerController)
{
}

void ANSM_GameMode::RequestRespawn(ACharacter* ElimintedCharacter, AController* EliminatedCharacterController)
{
}

AActor* ANSM_GameMode::GetPlayerSpawnArea(UNSM_GameInstance* GameInstance, FGameplayTag TeamTag)
{
	return nullptr;
}

void ANSM_GameMode::RegisterExistingPlayers()
{
	for (auto It = this->GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
        
		FUniqueNetIdRepl UniqueNetIdRepl;
		if (PlayerController->IsLocalPlayerController())
		{
			ULocalPlayer *LocalPlayer = PlayerController->GetLocalPlayer();
			if (IsValid(LocalPlayer))
			{
				UniqueNetIdRepl = LocalPlayer->GetPreferredUniqueNetId();
			}
			else
			{
				UNetConnection *RemoteNetConnection = Cast<UNetConnection>(PlayerController->Player);
				check(IsValid(RemoteNetConnection));
				UniqueNetIdRepl = RemoteNetConnection->PlayerId;
			}
		}
		else
		{
			UNetConnection *RemoteNetConnection = Cast<UNetConnection>(PlayerController->Player);
			check(IsValid(RemoteNetConnection));
			UniqueNetIdRepl = RemoteNetConnection->PlayerId;
		}

		// Get the unique player ID.
		TSharedPtr<const FUniqueNetId> UniqueNetId = UniqueNetIdRepl.GetUniqueNetId();
		check(UniqueNetId != nullptr);

		// Get the online session interface.
		IOnlineSubsystem *Subsystem = Online::GetSubsystem(PlayerController->GetWorld());
		IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

		// Register the player with the "MyLocalSessionName" session; this name should match the name you provided in CreateSession.
		if (!Session->RegisterPlayer(CustomSessionName, *UniqueNetId, false))
		{
			// The player could not be registered; typically you will want to kick the player from the server in this situation.
		}
	}

	this->bAllExistingPlayersRegistered = true;
}
