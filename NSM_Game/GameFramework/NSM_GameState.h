#pragma once

#include "CoreMinimal.h"
#include "NSM_PlayerController.h"
#include "NSM_PlayerStateBase.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameState.h"
#include "NordicSimMap/Core/NSM_NordicData.h"
#include "NSM_GameState.generated.h"

UCLASS()
class NORDICSIMMAP_API ANSM_GameState : public AGameState
{
	GENERATED_BODY()

public:
	ANSM_GameState();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	void InitializePlayerTeam();

	virtual float GetServerTime() const;

	void SetClientServerDeltaTime(float Value) { ClientServerDeltaTime = Value; }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EndGameObjectiveDestroyed(const FGameplayTag& DestroyedTag);

private:
	float TimeRemaining = 120.f;
	uint32 CountdownInt = 0;

	float ClientServerDeltaTime = 0.f;

	float TimeSyncFrequency = 5.f;
	float TimeSyncRunningTime = 4.f;
};
