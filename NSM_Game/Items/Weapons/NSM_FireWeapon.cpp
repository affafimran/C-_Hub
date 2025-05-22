#include "NSM_FireWeapon.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Items/NSM_ItemDataAsset.h"

ANSM_FireWeapon::ANSM_FireWeapon()
{
	AActor::SetReplicateMovement(true);
	bReplicates = true;
}

void ANSM_FireWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSM_FireWeapon, Particle);
}

void ANSM_FireWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(!IsValid(SkeletalMeshComponent))
	{
		return;
	}
	if(!IsValid(ItemData) && !IsValid(ItemData->WeaponAnimClass))
	{
		return;
	}
	SkeletalMeshComponent->SetAnimInstanceClass(ItemData->WeaponAnimClass);
}

void ANSM_FireWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ANSM_FireWeapon::Interact_Implementation(AActor* ActorInteracting)
{
	Super::Interact_Implementation(ActorInteracting);
}

void ANSM_FireWeapon::OnRep_ItemState()
{
	Super::OnRep_ItemState();
}

void ANSM_FireWeapon::Multicast_SetParticle_Implementation(UParticleSystem* NewParticle)
{
	Particle = NewParticle;
}

void ANSM_FireWeapon::Server_SetParticle_Implementation(UParticleSystem* NewParticle)
{
	Multicast_SetParticle(NewParticle);
}

void ANSM_FireWeapon::Multicast_SpawnWeaponTrail_Implementation(UWorld* World, const FVector TraceStart, const FVector TraceEnd)
{
	if(IsValid(Particle))
	{
		if(UParticleSystemComponent* Trail = UGameplayStatics::SpawnEmitterAtLocation(this, Particle, TraceStart); IsValid(Trail))
		{
			Trail->SetVectorParameter(FName("Target"),	TraceEnd);
		}
	}
}

void ANSM_FireWeapon::Server_SpawnWeaponTrail_Implementation(UWorld* World, const FVector TraceStart, const FVector TraceEnd)
{
	Multicast_SpawnWeaponTrail(World, TraceStart, TraceEnd);
}
