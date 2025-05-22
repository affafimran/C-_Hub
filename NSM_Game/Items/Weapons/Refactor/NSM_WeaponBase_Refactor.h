#pragma once

#include "CoreMinimal.h"
#include "NordicSimMap/Items/NSM_ItemBase.h"
#include "NSM_WeaponBase_Refactor.generated.h"

class UNiagaraSystem;
class ULegacyCameraShake;
class UNSM_WeaponComponent;
/**
 * 
 */
UCLASS()
class NORDICSIMMAP_API ANSM_WeaponBase_Refactor : public ANSM_ItemBase
{
	GENERATED_BODY()

public:	
	ANSM_WeaponBase_Refactor();
	virtual void UseLocal() override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual bool HasAmmo();
	virtual void ConsumeAmmo();
	virtual void WeaponShotRecoil();
	virtual void WeaponShotCamShake();
	
	virtual void InitializeItem() override;
	
	UFUNCTION(Server, Reliable)
	void Server_InitializeItem();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_InitializeItem();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestReload();

	UFUNCTION(BlueprintCallable)
	void Notify_OnReloadComplete();

	void SetWeaponSpread(float TargetSpread) { WeaponSpread = TargetSpread;}
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const;

	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	virtual bool GetShotLocationSocketTransform(FTransform& outTransform) const;

protected:	
	
	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector& TraceHitTarget, bool MissedShot = false);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Fire(const FVector& TraceHitTarget, bool MissedShot = false);

	virtual void PlayFireAnimation();

	virtual void OnFire(const FVector& TraceHitTarget, bool MissedShot = false);
	
	bool GetHitFromPlayer(FHitResult& HitResult, FVector& EndLocation);

	void GetHitFromAI(FVector& EndLocation, bool& MissShot, FVector StartLocation, ANSM_CharacterBase* WeaponHolster);
	
	UPROPERTY(Transient)
	TSubclassOf<class ANSM_MainProjectile> ProjectileClass;
	
	UPROPERTY(Transient)
	USkeletalMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UNSM_WeaponComponent* WeaponComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = true))
	TSubclassOf<ULegacyCameraShake> CameraShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = true))
	UParticleSystem* Tracer;
	
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = true))
	FVector TargetShot = FVector::ZeroVector;

	float WeaponSpread = 0.f;
	
	UPROPERTY(Replicated, Transient)
	FRandomStream RandomStream;

	FTimerHandle ShotTimerHandle;

	UFUNCTION()
	void OnShotTimerExpired();
};
