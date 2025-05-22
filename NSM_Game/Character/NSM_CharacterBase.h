#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "NSM_CharacterDataAsset.h"
#include "SignificanceManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "NordicSimMap/Components/Formations/NSM_FormationUnitComponent.h"
#include "NordicSimMap/Components/Inventory/NSM_InventoryComponent.h"
#include "NordicSimMap/Components/RTS/NSM_RTSActorSelectionComponent.h"
#include "NordicSimMap/Components/Shield/NSM_ShieldComponent.h"
#include "NordicSimMap/Components/Team/NSM_TeamComponent.h"
#include "NordicSimMap/Core/NSM_NordicData.h"
#include "NordicSimMap/Interface/NSM_InteractWithCrosshair.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "NSM_CharacterBase.generated.h"

class UNSM_BaseWidget;
class USphereComponent;
class ANSM_RTSPawn;
class UNSM_BaseGameplayAbility;
class UNSM_AbilitySystemComponent;
class UNSM_AttributeSet;
class UNSM_CharacterDataAsset;
/**
 * The Character class for Nordic.
 */
UCLASS()
class NORDICSIMMAP_API ANSM_CharacterBase : public ACharacter,  public IAbilitySystemInterface, public INSM_InteractWithCrosshair
{
	GENERATED_BODY()

public:
	ANSM_CharacterBase(const FObjectInitializer& ObjectInitializer);

	void StartFireTimer();

	void FinishFireAnimationTimer();
protected:
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category= "GAS|Tags")
	FGameplayTag FallDamageEventTag;
	
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category= "GAS|Tags")
	FGameplayTagContainer InAirEventTag;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category= "Character|Info")
	UNSM_CharacterDataAsset* CharacterDataAsset;
	
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category= "Character|Info")
	TSubclassOf<UGameplayEffect> CrouchEffect;
	
	UPROPERTY(Replicated)
	uint8 bAbilitiesInitialized : 1;

	UPROPERTY(ReplicatedUsing = OnRep_CharacterData)
	FNSM_CharacterData CharacterData;
	
	UFUNCTION(BlueprintCallable)
	UNSM_AbilitySystemComponent* GetCharacterAbilityComponent() const {return AbilitySystemComponent;}
	
	UFUNCTION(BlueprintCallable)
	UNSM_AttributeSet* GetCharacterAttributes() const {return Attributes;}
	
	UFUNCTION()
	void OnRep_CharacterData();

	UFUNCTION()
	virtual void InitFromCharacterData(const FNSM_CharacterData& InCharacterData, bool bFromReplication = false);
	
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void SetupASCInput();
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	void AddStartupGameplayAbilities();
	
public:
	
	UPROPERTY(Replicated, EditAnywhere)
	float MoveScale;
	
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;
	
	UPROPERTY(Replicated, EditAnywhere, Category = "Character Movement")
	float AngleToTurn;

	UPROPERTY(EditAnywhere)
	float CameraThreshold;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bCanAim;

	UPROPERTY(Replicated)
	bool bIsFiring;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	float FinishFireTime;

	FTimerHandle FinishFireTimer;
	
	TObjectPtr<UNSM_AbilitySystemComponent> AbilitySystemComponent;

	TObjectPtr<UNSM_AttributeSet> Attributes;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Cam;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CamArm;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RTSPawnLocation;
	
	UPROPERTY(Replicated,VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
	ANSM_RTSPawn* RTSPawnRef;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNSM_TeamComponent* TeamComponent;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNSM_FormationUnitComponent* FormationUnitComponent;
	
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAIPerceptionStimuliSourceComponent* StimuliSourceComponent;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Components")
	UNSM_RTSActorSelectionComponent* RTSActorSelectionComponent;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Action")
	UNSM_ActionComponent* ActionComponent;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Action")
	UNSM_ShieldComponent* ShieldComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "AI")
	UNSM_CommandComponent* CommandComponent;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	UWidgetComponent* CommandMarkerComponent;
	
	/** RTS Character Selection Decal  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSSelectionDecal")
	UNiagaraComponent* RTSSelectionVFX;
	
	ENSM_TurningInPlace TurningInPlace = ENSM_TurningInPlace::NotTurning;

	void UpdateAimOffset(float DeltaTime);
	void TurnInPlace(float DeltaTime);
	void CalculateSpread(float DeltaTime);
	void UpdateFOV(float DeltaTime);
	void UpdateCommanderWidget() const;

	ENSM_TurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	float GetTurningYaw() const;

	UFUNCTION(Server, Reliable)
	void Multicast_SetCommanderWidgetVisible(bool bVisible, bool bForce = false);
	
	/** Returns Character's cam FOV  */
	
	UFUNCTION()
	void OnTeamAssigned(const bool bIsLeader);

	UFUNCTION(BlueprintCallable)
	void SetCanAim(const bool CanAim);

	UFUNCTION(Server, Reliable)
	void Server_SetIsFiring(const bool IsFiring);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetIsFiring(const bool IsFiring);

	UFUNCTION()
	void HideCameraIfCharacterClose();

	UFUNCTION(Server, Reliable)
	void ServerSetCanAim(const bool CanAim);

	UFUNCTION(Server, Reliable)
	void ServerSetMovementSpeed(float Speed);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetMovementSpeed(float Speed);

	UFUNCTION(Server, Reliable)
	void Server_PlayAnimMontage(UAnimMontage* Montage);

	UFUNCTION(Server, Reliable)
	void Server_SetIsSquadLeader(bool IsSquadLeader);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetIsSquadLeader(bool IsSquadLeader);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAnimMontage(UAnimMontage* Montage);

	void NSMPlayAnimMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable)
	virtual UCameraComponent* GetFollowCamera() const {return Cam;}
	
	UFUNCTION(BlueprintCallable)
	virtual UNSM_CharacterDataAsset* GetCharacterDataAsset() const {return CharacterDataAsset;}
	
	UFUNCTION(BlueprintCallable)
	FNSM_CharacterData GetCharacterData() const {return CharacterData;}
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SetCharacterDataAsset(UNSM_CharacterDataAsset* NewCharacterDataAsset);
	
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void Multicast_SetCharacterDataAsset(UNSM_CharacterDataAsset* NewCharacterDataAsset);
	
	UFUNCTION(BlueprintCallable)
	void SetCharacterData(FNSM_CharacterData NewCharacterData);
	
	// RTS Function to test actors' selection
	UFUNCTION(BlueprintImplementableEvent)
	void BP_RTSActorSelected(bool SelectionActive);
	
	virtual ANSM_CharacterBase* GetCharacterOwner() {return this;}
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	void MoveForward(const struct FInputActionValue& ActionValue);
	void MoveRight(const struct FInputActionValue& ActionValue);
	void HorizontalRotation(const FInputActionValue& ActionValue);
	void SwitchWeapon(const FInputActionValue& ActionValue);
	void SwitchWeaponWithWheel(const FInputActionValue& ActionValue);
	void VerticalRotation(const FInputActionValue& ActionValue);
	void DropItem(const FInputActionValue& ActionValue);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_StartSprint();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_StopSprint();

	UFUNCTION(BlueprintCallable)
	void RequestCrouch();

	UFUNCTION(BlueprintCallable)
	void RequestUncrouch();

	UFUNCTION(Server,Reliable)
	void Server_SetAIController(ANSM_AIControllerBase* NewAIController);

	UFUNCTION(NetMulticast,Reliable)
	void Multicast_SetAIController(ANSM_AIControllerBase* NewAIController);
	
	UFUNCTION(BlueprintCallable)
	void RequestAimDownSight();

	UFUNCTION(BlueprintCallable)
	void RequestAimHip();

	UFUNCTION(BlueprintCallable)
	ANSM_AIControllerBase* GetAIController() const {return AIController;}
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category= "Skins")
	TArray<UMaterialInterface*> TeamOne;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category= "Skins")
	TArray<UMaterialInterface*> TeamTwo;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category= "Skins")
	TArray<UMaterialInterface*> TeamAI;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Squads")
	UNSM_BaseWidget* SquadWidget;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Squads")
	bool bSquadLeader;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Squads")
	bool bRogueAIAgent;

	bool bIsDead = false;
	
private:
	float TurningYaw;
	float InterpTurningYaw;
	FRotator InitialAimRotation;

	float AOYaw = 0.f;
	float InterpAO_Yaw = 0.f;
	float AOPitch = 0.f;

	UPROPERTY(Replicated)
	bool bIsSprinting = false;

private:
	void WeaponSetAiming(bool bVal) const;
	
	UPROPERTY(Replicated, EditDefaultsOnly, Category= "CharacterType")
	ANSM_AIControllerBase* AIController;
	
	float VelocitySpreadFactor;		
	float InAirSpreadFactor;
	float ADSSpreadFactor;		
	float ShotSpreadFactor;

	float DefaultFOV;
	float CurrentFOV;

	FVector OriginalCamLocation;
public:
	
	float GetAOYaw() const { return AOYaw; }
	float GetAOPitch() const { return AOPitch; }

	UFUNCTION(BlueprintPure)
	bool GetIsSprinting() const { return bIsSprinting; }

	void SetShotSpread(float Value) { ShotSpreadFactor = Value; }
	float GetShotSpread() const { return ShotSpreadFactor; }

	void NotifyHitmarker(bool bIsHeadshot = false);

private:
	float CalculateSignificance(
		USignificanceManager::FManagedObjectInfo* ObjectInfo,
		const FTransform& Viewpoint) const;
	
	void PostSignificance(
		const USignificanceManager::FManagedObjectInfo* ObjectInfo,
		float OldSignificance,
		float Significance,
		bool bFinal) const;
};




