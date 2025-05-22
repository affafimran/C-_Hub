#pragma once

#include "CoreMinimal.h"
#include "NSM_ItemBase.h"
#include "Engine/DataAsset.h"
#include "Niagara/Public/NiagaraComponent.h"
#include "NordicSimMap/Core/NSM_NordicData.h"
#include "Camera/CameraShake.h"
#include "Projectiles/NSM_MainProjectile.h"
#include "NSM_ItemDataAsset.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FScaledCascadeParticleSystem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UParticleSystem* ParticleSystem;
	
	UPROPERTY(EditAnywhere)
	FVector Scale{1.f};
};

UCLASS()
class NORDICSIMMAP_API UNSM_ItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual bool IsSupportedForNetworking() const override {return true;}
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSubclassOf<ANSM_ItemBase> ItemClass;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	ENSM_ItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float Quantity;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float UseRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float MaxQuantity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	bool bUsableWhileMoving;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	bool bShowInEquipment;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UNiagaraSystem* PickUpVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (EditCondition = "bIsStackable", EditConditionHides))
    int32 MaxStackSize;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    bool bUseSkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (EditCondition = "bUseSkeletalMesh", EditConditionHides))
    USkeletalMesh* SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (EditCondition = "!bUseSkeletalMesh", EditConditionHides))
    UStaticMesh* StaticMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TSubclassOf<UAnimInstance> ItemLayer;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* PickupAnimation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* HitReactionAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* DeathAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* EquipAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* UsableAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
    UAnimMontage* UnequipAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
    USoundBase* PickupSound;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
    USoundBase* EquipSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
    USoundBase* UnequipSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Effects")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Effects")
	TSubclassOf<UGameplayEffect> ShieldDamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Abilities")
	TArray<TSubclassOf<class UNSM_BaseGameplayAbility>> ItemAbilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float MaxWeaponDamage;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float HeadShotMultiplier = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float AttackRange;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float EffectiveRange = 4000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float BotDamageScaling = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float NeutralBotDamageScaling = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float CommanderBotDamageScaling = 0.75f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FGameplayTag MainAbilityTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FGameplayTag SecondaryAbilityTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FName EquipmentSocket;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FName ActiveWeaponSocket;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	ENSM_ItemPriority WeaponPriority;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	ENSM_WeaponType WeaponType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	TSubclassOf<UAnimInstance> WeaponAnimClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	ENSM_WeaponFireType WeaponFireType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bUseWidgetScope;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bHasInfiniteAmmo;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bIsAutomaticShot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bWeaponNeedRecoil = {true};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bWeaponNeedCameraShake = {true};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* Crosshair;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FName MuzzleSocket;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bNeedProjectile = {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	TSubclassOf<ANSM_MainProjectile> WeaponProjectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bWeaponCanZoom = {true};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ZoomDistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FVector ZoomLocDistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ReloadTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UAnimMontage* ReloadAnimation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UAnimMontage* WeaponUsableAnimation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UAnimMontage* WeaponReloadAnimation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float Recoil;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float CamShakeOscillationDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FROscillator CamShakeRotOscillation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FVOscillator CamShakeLocOscillation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	int32 MagazineSize;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	int32 MaxBulletsCapacity;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Weapon", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UNiagaraSystem* MuzzleFlashParticles;

	// Crosshair
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* CrosshairCenter;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* CrosshairLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* CrosshairRight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* CrosshairTop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* CrosshairBottom;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float BaseCrosshairSpread = 10.f;
	
	// Hitmarker
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitmarker", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* HitmarkerTopLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitmarker", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* HitmarkerTopRight;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitmarker", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* HitmarkerBottomLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitmarker", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UTexture2D* HitmarkerBottomRight;

	// Spread
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float BaseWeaponSpread = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float VelocitySpreadFactor = 3.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float InAirSpreadFactor = 2.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ToInAirSpreadSpeed = 2.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float InAirSpreadRecoverySpeed = 2.25f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ADSSpreadFactor = -0.75f;	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ToADSSpreadSpeed = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ToHipFireSpreadSpeed = 30.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ShotSpreadFactor = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float MaxShootingSpread = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ShotSpreadRecoverySpeed = 3.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float AIExtraSpread = 8.f;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Spread", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	bool bDebugSpread = false;

	// Zoom
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS Zoom", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ZoomFOV = 65.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS Zoom", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ZoomInSpeed = 20.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS Zoom", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float ZoomOutSpeed = 20.f;
	
	// Scope
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scope", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	TSubclassOf<AActor> ScopeClass = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scope", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FVector ScopeOffset = FVector::ZeroVector;
	
	// Decals	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Decals", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UMaterial* BulletDecalMaterial = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Decals", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	float DecalSize = 10.f;
	
	//
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Weapon VFX", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FScaledCascadeParticleSystem CascadeMuzzleFlash;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Weapon VFX", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	FScaledCascadeParticleSystem CascadeTracer;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Weapon VFX", meta = (EditCondition = "WeaponType == ENSM_WeaponType::FireWeapon && ItemType == ENSM_ItemType::Weapon", EditConditionHides))
	UNiagaraSystem* MatrixBullet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consumable", meta = (EditCondition = "ItemType == ENSM_ItemType::Consumable", EditConditionHides))
	ENSM_ConsumableType ConsumableType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (EditCondition = "ItemType == ENSM_ItemType::Consumable && ConsumableType == ENSM_ConsumableType::Ammo", EditConditionHides))
	ENSM_WeaponFireType AmmoType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throwable", meta = (EditCondition = "ItemType == ENSM_ItemType::Throwable", EditConditionHides))
	float ThrowForce;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throwable", meta = (EditCondition = "ItemType == ENSM_ItemType::Throwable", EditConditionHides))
	float ThrowableCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throwable", meta = (EditCondition = "ItemType == ENSM_ItemType::Throwable", EditConditionHides))
	UNiagaraSystem* ExplosionNiagaraSystem;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throwable", meta = (EditCondition = "ItemType == ENSM_ItemType::Throwable", EditConditionHides))
	bool bExplodeOnImpact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosive")
	float ExplosionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosive")
	float ExplosionDamage;
	
};
