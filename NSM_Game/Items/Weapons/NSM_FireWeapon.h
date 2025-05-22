#pragma once

#include "CoreMinimal.h"
#include "LegacyCameraShake.h"
#include "NSM_Weapon.h"
#include "NordicSimMap/Components/Weapon/NSM_WeaponComponent.h"
#include "NordicSimMap/Items/NSM_ItemBase.h"
#include "NSM_FireWeapon.generated.h"

/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_FireWeapon : public ANSM_Weapon
{
	GENERATED_BODY()
public:
	ANSM_FireWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	TSubclassOf<ULegacyCameraShake> CameraShakeClass;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Interact_Implementation(AActor* ActorInteracting) override;
	virtual void OnRep_ItemState() override;

	UPROPERTY(Replicated)
	UParticleSystem* Particle;

	UFUNCTION(Server, Reliable)
	void Server_SetParticle(UParticleSystem* NewParticle);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetParticle(UParticleSystem* NewParticle);
	
	UFUNCTION(Server, Reliable)
	void Server_SpawnWeaponTrail(UWorld* World, FVector TraceStart, FVector TraceEnd);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnWeaponTrail(UWorld* World, FVector TraceStart, FVector TraceEnd);
};
