#include "NSM_PlayerStateBase.h"
#include "NSM_GameInstance.h"
#include "NSM_GameMode.h"
#include "NSM_GameState.h"
#include "NSM_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Character/RTS/NSM_RTSPawn.h"
#include "NordicSimMap/TeamBase/SpawnArea/NSM_SpawnArea.h"


void ANSM_PlayerStateBase::InitializeSystem()
{
	InitializeSpawn();
	InitializePlayer();
	InitializeSquads();
}

void ANSM_PlayerStateBase::InitializeSpawn()
{
	if(!IsValid(GetWorld()) || !IsValid(GetGameInstance()) || !IsValid(GetPlayerController()))
	{
		return;
	}
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANSM_SpawnArea::StaticClass(), SpawnAreas);
	for (AActor* SpawnArea : SpawnAreas)
	{
		if(IsValid(SpawnArea))
		{
			const UNSM_TeamComponent* TeamComponent = SpawnArea->FindComponentByClass<UNSM_TeamComponent>();
			if(IsValid(TeamComponent))
			{
				if(TeamComponent->GetTeamTag() == PlayerTeamTag)
				{
					PlayerSquadSpawnAreas.Add(SpawnArea);
				}
			}
		}
	}
}

void ANSM_PlayerStateBase::InitializePlayer()
{
	if(!IsValid(GetWorld()))
	{
		return;
	}
	
	// Get the game mode from the world
	const ANSM_GameMode* GameMode = Cast<ANSM_GameMode>(GetWorld()->GetAuthGameMode());
	if(!IsValid(GameMode))
	{
		return;
	}
	
	ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(GetPlayerController());
	if(!IsValid(PlayerController))
	{
		return;
	}
	
	if(!IsValid(GetGameInstance()))
	{
		return;
	}
	const UNSM_GameInstance* GameInstance = Cast<UNSM_GameInstance>(GetGameInstance());
	if(!IsValid(GameInstance))
	{
		return;
	}
	if (GameInstance->PlayerClass == nullptr)
	{
		return;
	}
	
	if(PlayerSquadSpawnAreas.Num() <= 0)
	{
		return;
	}

	ANSM_SpawnArea* PlayerSpawnArea = Cast<ANSM_SpawnArea>(PlayerSquadSpawnAreas[0]);

	if(!IsValid(PlayerSpawnArea))
	{
		return;
	}
					
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	PlayerSpawnArea->GetRandomPointInSpawnArea(GameInstance->PlayerClass);
	const FVector PlayerSquadSpawn = PlayerSpawnArea->RandomPointToSpawn;
	ANSM_CharacterBase* PlayerCharacter = GetWorld()->SpawnActor<ANSM_CharacterBase>(GameInstance->PlayerClass, PlayerSquadSpawn + FVector(40.f, 0.f, 40.f), FRotator::ZeroRotator, SpawnParams);
	PlayerController->SetCharacterReference(PlayerCharacter);
	PlayerCharacter->RTSPawnRef = GetWorld()->SpawnActor<ANSM_RTSPawn>(GameInstance->RTSPawnClass, PlayerCharacter->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	PlayerController->SetRTSPawnReference(PlayerCharacter->RTSPawnRef);
	
	if(IsValid(PlayerCharacter->AIControllerClass))
	{
		PlayerCharacter->Server_SetAIController(GetWorld()->SpawnActor<ANSM_AIControllerBase>(PlayerCharacter->AIControllerClass));
	}

	PlayerController->Possess(PlayerCharacter);
	PlayerController->InitializeRTSMode();
    
	TArray<ANSM_CharacterBase*> SquadMembers;
	SquadMembers.Empty();
	SquadMembers.Add(PlayerCharacter);
	TArray<ANSM_CharacterBase*> SpawnedMembers = SpawnSquadMembers(GameInstance->SquadSize - 1, PlayerSquadSpawn, PlayerSpawnArea->CharacterDataAsset);
	for (ANSM_CharacterBase* SquadMember : SpawnedMembers)
	{
		if(IsValid(SquadMember))
		{
			SquadMembers.Add(SquadMember);
		}
	}
	CreateSquad(SquadMembers, true);
}

void ANSM_PlayerStateBase::InitializeSquads()
{
	if(!IsValid(GetGameInstance()))
	{
		return;
	}
	const UNSM_GameInstance* GameInstance = Cast<UNSM_GameInstance>(GetGameInstance());
	if(!IsValid(GameInstance))
	{
		return;
	}
	
	if(!IsValid(GetWorld()))
	{
		return;
	}

	const ANSM_GameMode* GameMode = Cast<ANSM_GameMode>(GetWorld()->GetAuthGameMode());
	if(!IsValid(GameMode))
	{
		return;
	}
	if(!IsValid(GetGameInstance()))
	{
		return;
	}
	
	for (int i = 1; i < GameInstance->SquadNumbers; i++)
	{
		if(!PlayerSquadSpawnAreas.IsValidIndex(i))
		{
			continue;
		}
		ANSM_SpawnArea* SquadSpawnArea = Cast<ANSM_SpawnArea>(PlayerSquadSpawnAreas[i]);
		SquadSpawnArea->GetRandomPointInSpawnArea(GameInstance->SquadMemberClass);
		const FVector RandomPosition = SquadSpawnArea->RandomPointToSpawn;
		TArray<ANSM_CharacterBase*> SquadMembers;
		SquadMembers.Empty();
		for (ANSM_CharacterBase* SquadMember : SpawnSquadMembers(GameInstance->SquadSize, RandomPosition, SquadSpawnArea->CharacterDataAsset))
		{
			if(IsValid(SquadMember))
			{
				SquadMembers.AddUnique(SquadMember);
			}
		}
		CreateSquad(SquadMembers);
	}
}

void ANSM_PlayerStateBase::AssignTeam(const int32 PlayerAmount)
{
	if(PlayerAmount % 2 == 1)
	{
		PlayerTeamTag = FGameplayTag::RequestGameplayTag(FName("Team.One"));
	}
	else
	{
		PlayerTeamTag = FGameplayTag::RequestGameplayTag(FName("Team.Two"));
	}
}

void ANSM_PlayerStateBase::CreateSquad(TArray<ANSM_CharacterBase*> Members, const bool bPlayerSquad)
{
	if (!IsValid(GetGameInstance()))
	{
		return;
	}
	const UNSM_GameInstance* GameInstance = Cast<UNSM_GameInstance>(GetGameInstance());
	if(!IsValid(GameInstance))
	{
		return;
	}
	ANSM_PlayerController* PlayerController = static_cast<ANSM_PlayerController*>(GetPlayerController());
	if(!IsValid(PlayerController))
	{
		return;
	}
	
	UNSM_FormationManagerComponent* FormationManagerComponent = nullptr;
	ANSM_CharacterBase* FirstCharacter = Cast<ANSM_CharacterBase>(Members[0]);
	FirstCharacter->bSquadLeader = true;
	if(bPlayerSquad)
	{
		if(IsValid(GetPlayerController()))
		{
			FormationManagerComponent = GetPlayerController()->FindComponentByClass<UNSM_FormationManagerComponent>();
		}
	}
	else
	{
		FormationManagerComponent = Cast<UNSM_FormationManagerComponent>(FirstCharacter->GetController()->AddComponentByClass(UNSM_FormationManagerComponent::StaticClass(), false, FTransform::Identity, false));
		FirstCharacter->GetController()->AddInstanceComponent(FormationManagerComponent);
	}
	
	if(!IsValid(FormationManagerComponent))
	{
		return;
	}
	FormationManagerComponent->OffensiveFormations = GameInstance->OffensiveFormations;
	FormationManagerComponent->StealthFormations = GameInstance->StealthFormations;

	FNSM_SquadData SquadData = {};
	for (ANSM_CharacterBase* Member : Members)
	{
		if(IsValid(Member))
		{
			UNSM_FormationUnitComponent* SquadComponent = Member->FindComponentByClass<UNSM_FormationUnitComponent>();
			if(IsValid(SquadComponent))
			{
				SquadData.SquadMembers = Members;
				SquadData.CurrentTeamTag = PlayerTeamTag;
				SquadData.LeaderCharacter = FirstCharacter;
				SquadData.MemberId = Members.Find(Member);
				SquadData.CurrentSquadId = SquadAmount;
				SquadData.ParentSquadId = SquadAmount;
				SquadData.bIsSubSquad = false;
				SquadComponent->Server_SetCurrentSquadData(SquadData);
				SquadComponent->Server_SetSquadManager(FormationManagerComponent);
				if(FormationManagerComponent->OffensiveFormations.Num() > 0)
				{
					SquadComponent->CurrentFormation = FormationManagerComponent->OffensiveFormations[0];
				}
			}

			if(Members[0] == Member) 
			{
				FormationManagerComponent->Squads.Add(SquadData);
			}
			
			ANSM_CharacterBase* MemberCharacter = Cast<ANSM_CharacterBase>(Member);
			ANSM_AIControllerBase* AIController = Cast<ANSM_AIControllerBase>(MemberCharacter->GetController());
			if (IsValid(AIController))
			{
				AIController->Possess(MemberCharacter);
			}
		}
	}
	PlayerController->Squads.Add(SquadData);
	SquadAmount++;
}

TArray<ANSM_CharacterBase*> ANSM_PlayerStateBase::SpawnSquadMembers(const int32 NumberOfMembers, const FVector& LocationToSpawn, UNSM_CharacterDataAsset* NewCharacterDataAsset)
{
	TArray<ANSM_CharacterBase*> ActorsSpawned;
	if (!IsValid(GetGameInstance()))
	{
		return ActorsSpawned;
	}
	const UNSM_GameInstance* GameInstance = Cast<UNSM_GameInstance>(GetGameInstance());
	if(!IsValid(GameInstance))
	{
		return ActorsSpawned;
	}
	if(!IsValid(GetWorld()))
	{
		return ActorsSpawned;
	}
    
	if (GameInstance->SquadMemberClass == nullptr)
	{
		return ActorsSpawned;
	}

	constexpr float SpawnRadius = 230.0f;
	for (int32 i = 0; i < NumberOfMembers; ++i)
	{
		const float RandomAngle = FMath::RandRange(0.0f, 250.0f);
		FVector Offset = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f) * SpawnRadius;
		FVector MemberSpawnLocation = LocationToSpawn + Offset;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ANSM_CharacterBase* MemberSpawned = GetWorld()->SpawnActor<ANSM_CharacterBase>(GameInstance->SquadMemberClass, MemberSpawnLocation, FRotator::ZeroRotator, SpawnParams);
		MemberSpawned->Server_SetCharacterDataAsset(NewCharacterDataAsset);
		ActorsSpawned.Add(MemberSpawned);
	}

	return ActorsSpawned;
}
