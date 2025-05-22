#include "NSM_GameInstance.h"
#include "AbilitySystemGlobals.h"
#include "NordicSimMap/GameFramework/NSM_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"


UNSM_GameInstance::UNSM_GameInstance()
{
	CustomSessionName = FName("NSM Session");
	SquadSize = 4;
	SquadNumbers = 9;
}

void UNSM_GameInstance::Init()
{
	Super::Init();

	const IOnlineSubsystem *Subsystem = Online::GetSubsystem(this->GetWorld());
	
	if (Subsystem)
	{		
		const IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	
		this->LoginDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(0, FOnLoginComplete::FDelegate::CreateUObject(this, &UNSM_GameInstance::HandleLoginComplete));
		if (!Identity->AutoLogin(0 /* LocalUserNum */))
		{
			// Call didn't start, return error.
		}
	}
	
	UAbilitySystemGlobals::Get().InitGlobalData();
}

void UNSM_GameInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNSM_GameInstance, PlayerClass);
	DOREPLIFETIME(UNSM_GameInstance, RTSPawnClass);
}

ANSM_PlayerController* UNSM_GameInstance::GetPlayerController()
{
	if (!IsValid(GetWorld()))
	{
		return nullptr;
	}

	if (IsValid(GetWorld()->GetFirstPlayerController()))
	{
		return PlayerController = Cast<ANSM_PlayerController>(GetWorld()->GetFirstPlayerController());
	}
	
	return nullptr;
}

void UNSM_GameInstance::HandleLoginComplete(int32 LocalUserNum,	bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error)
{
	// TODO: Check bWasSuccessful to see if the login was completed.

	// Deregister the event handler.
	const IOnlineSubsystem *Subsystem = Online::GetSubsystem(this->GetWorld());
	const IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, this->LoginDelegateHandle);
	this->LoginDelegateHandle.Reset();
}




