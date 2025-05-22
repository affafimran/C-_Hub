#include "NSM_CharacterBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameplayEffectExtension.h"
#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Components/RTS/NSM_RTSActorSelectionComponent.h"
#include "NordicSimMap/Core/NSM_GeneralStatics.h"
#include "NordicSimMap/GameFramework/NSM_PlayerController.h"
#include "NordicSimMap/GAS/NSM_AbilitySystemComponent.h"
#include "NordicSimMap/GAS/NSM_AttributeSet.h"
#include "NordicSimMap/GAS/NSM_BaseGameplayAbility.h"
#include "NordicSimMap/Animation/NSM_BaseAnimInstance.h"
#include "NordicSimMap/Components/Equipment/NSM_EquipmentComponent.h"
#include "RTS/NSM_RTSPawn.h"
#include "NordicSimMap/NordicSimMap.h"
#include "NordicSimMap/Components/Movement/NSM_CharacterMovementComponent.h"
#include "NordicSimMap/Components/Weapon/NSM_WeaponComponent.h"
#include "NordicSimMap/GameFramework/NSM_PlayerStateBase.h"
#include "NordicSimMap/Items/Weapons/Refactor/NSM_SniperRifle_Refactor.h"
#include "NordicSimMap/Items/Weapons/Refactor/NSM_WeaponBase_Refactor.h"
#include "NordicSimMap/UI/NSM_SquadLeaderWidget.h"

ANSM_CharacterBase::ANSM_CharacterBase(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UNSM_CharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	CamArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	CamArm->SetupAttachment(GetRootComponent());
	CamArm->TargetArmLength = 500.0f;
	CamArm->SetRelativeRotation(FRotator(-30.0f, 0.0f, 0.0f));

	CamArm->bEnableCameraLag = true;
	CamArm->CameraLagSpeed = 5;
	CamArm->CameraLagMaxDistance = 1.5;

	CamArm->bEnableCameraRotationLag = false;
	CamArm->CameraRotationLagSpeed = 4;
	CamArm->CameraLagMaxTimeStep = 1;
	
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Cam->SetupAttachment(CamArm);

	MoveScale = 1.0;

	// Replicate socket location
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	/** Create the Decal Component and attach the Decal Component to the root component of the actor */
	RTSSelectionVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SelectionComponent"));
	RTSSelectionVFX->SetupAttachment(GetMesh());
	RTSSelectionVFX->SetAutoActivate(false);
	RTSSelectionVFX->ComponentTags.Add(FName("Selection"));

	bAbilitiesInitialized = false;
	AbilitySystemComponent = CreateDefaultSubobject<UNSM_AbilitySystemComponent>(TEXT("AbilitySystemComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	Attributes = CreateDefaultSubobject<UNSM_AttributeSet>(TEXT("Attributes"));
	AbilitySystemComponent->AddSpawnedAttribute(Attributes);

	TeamComponent = CreateDefaultSubobject<UNSM_TeamComponent>(TEXT("TeamComp"));
	TeamComponent->SetIsReplicated(true);

	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	FormationUnitComponent = CreateDefaultSubobject<UNSM_FormationUnitComponent>(TEXT("FormationUnitComp"));
	FormationUnitComponent->SetIsReplicated(true);

	RTSActorSelectionComponent = CreateDefaultSubobject<UNSM_RTSActorSelectionComponent>(TEXT("ActorSelectionComponent"));

	ActionComponent = CreateDefaultSubobject<UNSM_ActionComponent>(TEXT("ActionComponent"));

	ShieldComponent = CreateDefaultSubobject<UNSM_ShieldComponent>(TEXT("ShieldComponent"));
	
	CommandComponent = CreateDefaultSubobject<UNSM_CommandComponent>(TEXT("CommandComp"));

	RTSPawnLocation = CreateDefaultSubobject<USceneComponent>(TEXT("RTSPawnLocation"));
	RTSPawnLocation->SetupAttachment(RootComponent);
	RTSPawnLocation->SetIsReplicated(true);
	
	CommandMarkerComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("CommandMarker"));
	CommandMarkerComponent->SetupAttachment(RootComponent);

	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->SetAvoidanceEnabled(true);
	GetCharacterMovement()->AvoidanceConsiderationRadius = 500.f;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	//GetCharacterMovement()->bUseControllerDesiredRotation = true;
	AngleToTurn = 90.f;
	TurnRateGamepad = 40.f;
	TurningInPlace = ENSM_TurningInPlace::NotTurning;
	InitialAimRotation = GetActorRotation();
	CameraThreshold = 400.f;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	FinishFireTime = 2.f;
	bReplicates = true;
	bCanAim = false;
	AActor::SetReplicateMovement(true);
}

void ANSM_CharacterBase::StartFireTimer()
{
	if(!IsValid(GetWorld()))
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(FinishFireTimer, this, &ANSM_CharacterBase::FinishFireAnimationTimer, FinishFireTime, false);
}

void ANSM_CharacterBase::FinishFireAnimationTimer()
{
	if(!IsValid(GetWorld()) || bCanAim)
	{
		GetWorld()->GetTimerManager().ClearTimer(FinishFireTimer);
		Server_SetIsFiring(false);
		return;
	}
	
	GetWorld()->GetTimerManager().ClearTimer(FinishFireTimer);
	Server_SetIsFiring(false);
	SetCanAim(false);
}
void ANSM_CharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Cam))
	{
		DefaultFOV = Cam->FieldOfView;
		CurrentFOV = DefaultFOV;
		OriginalCamLocation = Cam->GetRelativeLocation();
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if(IsValid(CameraManager))
	{
		CameraManager->ViewPitchMax = 40.f;
		CameraManager->ViewPitchMin = -80.f;
	}

	bRogueAIAgent = ActorHasTag("EnemyAI");

	if (USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(GetWorld()))
	{
		const USignificanceManager::FManagedObjectSignificanceFunction SignificanceFunction =
			[&] (USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint) -> float
			{
				return CalculateSignificance(ObjectInfo, Viewpoint);
			};

		const USignificanceManager::FManagedObjectPostSignificanceFunction PostSignificanceFunction =
			[&] (USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
			{
				PostSignificance(ObjectInfo, OldSignificance, Significance, bFinal);
			};

		SignificanceManager->RegisterObject(
			this,
			TEXT("NSM_Character"),
			SignificanceFunction,
			USignificanceManager::EPostSignificanceType::Concurrent,
			PostSignificanceFunction
			);
	}

	//GetCharacterMovement()->SetGroundMovementMode(IsPlayerControlled() || !HasAuthority() ? MOVE_Walking : MOVE_NavWalking);
	GetCharacterMovement()->SetGroundMovementMode(MOVE_Walking);
}

void ANSM_CharacterBase::PossessedBy(AController *NewController)
{
	Super::PossessedBy(NewController);
	
	// Initialize Gas in server
	if (AbilitySystemComponent && !bAbilitiesInitialized)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		if(IsValid(CharacterDataAsset))
		{
			CharacterData = CharacterDataAsset->CharacterData;
		}
		AddStartupGameplayAbilities();
	}
	
	OnTeamAssigned(true);
	
	//GetCharacterMovement()->SetGroundMovementMode(IsPlayerControlled() || !HasAuthority() ? MOVE_Walking : MOVE_NavWalking);
	GetCharacterMovement()->SetGroundMovementMode(MOVE_Walking);

	if (USkeletalMeshComponentBudgeted* SKM = Cast<USkeletalMeshComponentBudgeted>(GetMesh()))
	{
		if (IAnimationBudgetAllocator* Interface = IAnimationBudgetAllocator::Get(GetWorld()))
		{
			if (IsPlayerControlled())
			{				
				Interface->UnregisterComponent(SKM);
			}
			else
			{
				Interface->RegisterComponent(SKM);
			}
		}
	}
}

void ANSM_CharacterBase::UnPossessed()
{
	Super::UnPossessed();
}

void ANSM_CharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Initialize Gas in server
	if (AbilitySystemComponent && !bAbilitiesInitialized)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		if(IsValid(CharacterDataAsset))
		{
			CharacterData = CharacterDataAsset->CharacterData;
		}
		AddStartupGameplayAbilities();
	}
	OnTeamAssigned(true);
}

void ANSM_CharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//HideCameraIfCharacterClose();
	UpdateAimOffset(DeltaTime);
	CalculateSpread(DeltaTime);
	UpdateFOV(DeltaTime);
	UpdateCommanderWidget();
}

// Called to bind functionality to input
void ANSM_CharacterBase::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent *EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const ANSM_PlayerController *NPC = GetController<ANSM_PlayerController>();
	check(EIC && NPC);
	EIC->BindAction(NPC->MoveForwardAction, ETriggerEvent::Triggered, this, &ANSM_CharacterBase::MoveForward);
	EIC->BindAction(NPC->MoveRightAction, ETriggerEvent::Triggered, this, &ANSM_CharacterBase::MoveRight);
	EIC->BindAction(NPC->VerticalRotateAction, ETriggerEvent::Triggered, this, &ANSM_CharacterBase::VerticalRotation);
	EIC->BindAction(NPC->HorizontalRotateAction, ETriggerEvent::Triggered, this, &ANSM_CharacterBase::HorizontalRotation);
	EIC->BindAction(NPC->SwitchWeaponAction, ETriggerEvent::Started, this, &ANSM_CharacterBase::SwitchWeapon);
	EIC->BindAction(NPC->SwitchWeaponWheelAction, ETriggerEvent::Started, this, &ANSM_CharacterBase::SwitchWeaponWithWheel);
	EIC->BindAction(NPC->DropItemAction, ETriggerEvent::Started, this, &ANSM_CharacterBase::DropItem);
	EIC->BindAction(NPC->SprintAction, ETriggerEvent::Started, this, &ANSM_CharacterBase::Server_StartSprint);
	EIC->BindAction(NPC->SprintAction, ETriggerEvent::Completed, this, &ANSM_CharacterBase::Server_StopSprint);
	EIC->BindAction(NPC->CrouchAction, ETriggerEvent::Started, this, &ANSM_CharacterBase::RequestCrouch);
	EIC->BindAction(NPC->CrouchAction, ETriggerEvent::Completed, this, &ANSM_CharacterBase::RequestUncrouch);
	EIC->BindAction(NPC->AimDownSightAction, ETriggerEvent::Started, this, &ANSM_CharacterBase::RequestAimDownSight);
	EIC->BindAction(NPC->AimDownSightAction, ETriggerEvent::Completed, this, &ANSM_CharacterBase::RequestAimHip);

	SetupASCInput();

	const ULocalPlayer *LocalPlayer = NPC->GetLocalPlayer();
	check(LocalPlayer);
	UEnhancedInputLocalPlayerSubsystem *Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(NPC->PawnMappingContext, 0);
}

void ANSM_CharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(CharacterDataAsset))
	{
		SetCharacterData(CharacterDataAsset->CharacterData);
	}

	UNSM_EquipmentComponent *EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->SetIsReplicated(true);
	}
	OnTeamAssigned(true);
}

void ANSM_CharacterBase::SetupASCInput()
{
	if (IsValid(AbilitySystemComponent) && IsValid(InputComponent))
	{
		const FGameplayAbilityInputBinds Binds(
			"ConfirmTarget",
			"CancelTarget",
			"ENSM_AbilityInputID",
			static_cast<int32>(ENSM_AbilityInputID::ConfirmTarget),
			static_cast<int32>(ENSM_AbilityInputID::CancelTarget));

		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
	}
}

void ANSM_CharacterBase::Landed(const FHitResult &Hit)
{
	Super::Landed(Hit);
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->RemoveActiveEffectsWithTags(InAirEventTag);
	}

	if (!IsValid(GetCharacterMovement()) || !GetCharacterMovement()->IsFalling())
	{
		return;
	}

	const float FallSpeed = GetVelocity().Z * -1;
	float DamageAmount = 0.f;

	if (FallSpeed > 4000.f && FallSpeed < 5000.f)
	{
		DamageAmount = FallSpeed / 100;
	}
	else if (FallSpeed >= 2000.f)
	{
		DamageAmount = 100;
	}

	if(DamageAmount == 0.f)
	{
		return;
	}
	
	TSubclassOf<UGameplayEffect> GameplayEffect;
	for (const TSubclassOf<UGameplayEffect> EffectClass : CharacterData.Effects)
	{
		const UGameplayEffect *EffectDefault = EffectClass->GetDefaultObject<UGameplayEffect>();
		if (EffectDefault && EffectDefault->InheritableGameplayEffectTags.CombinedTags.HasTagExact(FallDamageEventTag))
		{
			GameplayEffect = EffectClass;
			break;
		}
	}

	if (!IsValid(GameplayEffect))
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(Hit, true);
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, 1, Context);
	SpecHandle.Data.Get()->SetSetByCallerMagnitude(FallDamageEventTag, (DamageAmount * -1));
	if (SpecHandle.IsValid())
	{
		FActiveGameplayEffectHandle EffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ANSM_CharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (!IsValid(CrouchEffect.Get()))
		return;

	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	Server_StopSprint();

	const FGameplayEffectContextHandle EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(CrouchEffect, 1, EffectContextHandle);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ANSM_CharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (IsValid(AbilitySystemComponent) && IsValid(CrouchEffect.Get()))
	{
		AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(CrouchEffect, AbilitySystemComponent);
	}
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void ANSM_CharacterBase::OnRep_CharacterData()
{
	InitFromCharacterData(CharacterData, true);
}

void ANSM_CharacterBase::InitFromCharacterData(const FNSM_CharacterData &InCharacterData, bool bFromReplication)
{
}

UAbilitySystemComponent *ANSM_CharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANSM_CharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSM_CharacterBase, bAbilitiesInitialized);
	DOREPLIFETIME(ANSM_CharacterBase, bSquadLeader);
	DOREPLIFETIME(ANSM_CharacterBase, TeamComponent);
	DOREPLIFETIME(ANSM_CharacterBase, RTSPawnLocation);
	DOREPLIFETIME(ANSM_CharacterBase, FormationUnitComponent);
	DOREPLIFETIME(ANSM_CharacterBase, RTSActorSelectionComponent);
	DOREPLIFETIME(ANSM_CharacterBase, ActionComponent);
	DOREPLIFETIME(ANSM_CharacterBase, bCanAim);
	DOREPLIFETIME(ANSM_CharacterBase, bIsSprinting);
	DOREPLIFETIME(ANSM_CharacterBase, CharacterData);
	DOREPLIFETIME(ANSM_CharacterBase, CharacterDataAsset);
	DOREPLIFETIME(ANSM_CharacterBase, TeamAI);
	DOREPLIFETIME(ANSM_CharacterBase, TeamOne);
	DOREPLIFETIME(ANSM_CharacterBase, TeamTwo);
}

void ANSM_CharacterBase::AddStartupGameplayAbilities()
{
	if (!IsValid(AbilitySystemComponent) || bAbilitiesInitialized)
	{
		return;
	}

	for (const TSubclassOf<UNSM_BaseGameplayAbility> &StartupAbility : CharacterData.Abilities)
	{
		if (IsValid(StartupAbility))
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(
				StartupAbility,
				1,
				static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputId),
				this));
		}
	}

	if(!IsValid(Attributes))
	{
		return;
	}
	
	TArray<TSubclassOf<UGameplayEffect>> LocalEffects = CharacterData.Effects;
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect> &GameplayEffect : LocalEffects)
	{
		if (IsValid(GameplayEffect))
		{
			if(!IsValid(GameplayEffect.Get()))
			{
				continue;
			}
			FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, 1, EffectContext);

			if(NewHandle.Data.Get()->Modifiers.IsEmpty())
			{
				continue;
			}
			
			if (NewHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
			}
		}
	}

	GetCharacterMovement()->MaxWalkSpeed = Attributes->GetWalkSpeed();
	GetCharacterMovement()->MaxWalkSpeedCrouched = Attributes->GetCrouchSpeed();

	bAbilitiesInitialized = true;
}

void ANSM_CharacterBase::MoveForward(const FInputActionValue &ActionValue)
{
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, ActionValue.Get<float>());
	}
}

void ANSM_CharacterBase::MoveRight(const FInputActionValue &ActionValue)
{
	if (Controller != nullptr)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, ActionValue.Get<float>());
	}
}

void ANSM_CharacterBase::HorizontalRotation(const FInputActionValue &ActionValue)
{
	if (ActionValue[0])
	{
		AddControllerYawInput(ActionValue[0] * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
	}
}

void ANSM_CharacterBase::VerticalRotation(const FInputActionValue &ActionValue)
{
	if (ActionValue[0])
	{
		AddControllerPitchInput(ActionValue[0] * -TurnRateGamepad * GetWorld()->GetDeltaSeconds());
	}
}

void ANSM_CharacterBase::UpdateAimOffset(float DeltaTime)
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();	

	if (Speed == 0.f && !bIsInAir && bCanAim) // standing still, not jumping
	{
		const FRotator& CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator& DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, InitialAimRotation);
		AOYaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ENSM_TurningInPlace::NotTurning)
		{
			InterpAO_Yaw = AOYaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	else
	{
		InitialAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AOYaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ENSM_TurningInPlace::NotTurning;
	}
	
	AOPitch = GetBaseAimRotation().Pitch;
	
	if (AOPitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AOPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AOPitch);
	}
}

void ANSM_CharacterBase::TurnInPlace(float DeltaTime)
{
	if (AOYaw > AngleToTurn)
	{
		TurningInPlace = ENSM_TurningInPlace::TurningRight;
	}

	if (AOYaw < -AngleToTurn)
	{
		TurningInPlace = ENSM_TurningInPlace::TurningLeft;
	}

	if (TurningInPlace != ENSM_TurningInPlace::NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 3.f);
		AOYaw = InterpAO_Yaw;

		if (FMath::Abs(AOYaw) < 15.f)
		{
			TurningInPlace = ENSM_TurningInPlace::NotTurning;
			InitialAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ANSM_CharacterBase::CalculateSpread(float DeltaTime)
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	ANSM_BaseHUD* HUD = PlayerController ? Cast<ANSM_BaseHUD>(PlayerController->GetHUD()) : nullptr;
	const UNSM_EquipmentComponent * EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (!EquipmentComponent)
	{
		return;
	}
			
	const UNSM_ItemDataAsset* ItemData = nullptr;
	ANSM_WeaponBase_Refactor* ActiveWeapon = Cast<ANSM_WeaponBase_Refactor>(EquipmentComponent->GetActiveItem());
	ItemData = ActiveWeapon ? ActiveWeapon->GetItemData() : nullptr;

	if (!ItemData)
	{
		return;
	}

	if (HUD && !ItemData)
	{
		HUD->GetHUDPackage().Reset();
		return;
	}
						
	const FVector2D WalkSpeedRange(0.f, 600.f);
	const FVector2D VelocityMultiplierRange(0.f, ItemData->VelocitySpreadFactor);
	FVector Velocity = GetVelocity();			
	Velocity.Z = 0.f;
			
	VelocitySpreadFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange,Velocity.Size());
	if (GetCharacterMovement()->IsFalling())
	{
		InAirSpreadFactor = FMath::FInterpTo(InAirSpreadFactor, ItemData->InAirSpreadFactor, DeltaTime, ItemData->ToInAirSpreadSpeed);
	}
	else
	{			
		InAirSpreadFactor = FMath::FInterpTo(InAirSpreadFactor, 0.f, DeltaTime, ItemData->InAirSpreadRecoverySpeed);
	}
			
	if (bCanAim)
	{
		ADSSpreadFactor = FMath::FInterpTo(ADSSpreadFactor, ItemData->ADSSpreadFactor , DeltaTime, ItemData->ToADSSpreadSpeed);
	}
	else
	{
		ADSSpreadFactor = FMath::FInterpTo(ADSSpreadFactor, 0.f , DeltaTime, ItemData->ToHipFireSpreadSpeed);
	}
				
	ShotSpreadFactor = FMath::FInterpTo(ShotSpreadFactor, 0.f, DeltaTime, ItemData->ShotSpreadRecoverySpeed);

	float TotalSpread = ItemData->BaseWeaponSpread + VelocitySpreadFactor + InAirSpreadFactor + ADSSpreadFactor + ShotSpreadFactor;;

	if (!IsPlayerControlled())
	{				
		TotalSpread += ItemData->AIExtraSpread;
	}
			
	ActiveWeapon->SetWeaponSpread(TotalSpread);

	if (HUD)
	{
		FHUDPackage& HUDPackage = HUD->GetHUDPackage();
		HUDPackage.BaseCrosshairSpread = ItemData->BaseCrosshairSpread;
		HUDPackage.CrosshairSpread = TotalSpread; 
		HUDPackage.CrosshairCenter = ItemData->CrosshairCenter;
		HUDPackage.CrosshairLeft = ItemData->CrosshairLeft;
		HUDPackage.CrosshairRight = ItemData->CrosshairRight;
		HUDPackage.CrosshairBottom = ItemData->CrosshairBottom;
		HUDPackage.CrosshairTop = ItemData->CrosshairTop;

		// Calculate HitMarker
		if (HUDPackage.HitmarkerSpread > -UE_KINDA_SMALL_NUMBER)
		{
			HUDPackage.HitmarkerSpread += DeltaTime * 5.f;			
		}
		
		HUDPackage.HitmarkerTopLeft = ItemData->HitmarkerTopLeft;
		HUDPackage.HitmarkerTopRight = ItemData->HitmarkerTopRight;
		HUDPackage.HitmarkerBottomLeft = ItemData->HitmarkerBottomLeft;
		HUDPackage.HitmarkerBottomRight = ItemData->HitmarkerBottomRight;
	}
}

void ANSM_CharacterBase::UpdateFOV(float DeltaTime)
{
	if (!IsLocallyControlled() || !IsPlayerControlled())
	{
		return;
	}
	
	UCameraComponent* CameraComponent = GetFollowCamera();
	const UNSM_EquipmentComponent* EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (!EquipmentComponent || !CameraComponent)
	{
		return;
	}
			
	const UNSM_ItemDataAsset* ItemData = nullptr;
	ANSM_WeaponBase_Refactor* ActiveWeapon = Cast<ANSM_WeaponBase_Refactor>(EquipmentComponent->GetActiveItem());
	ItemData = ActiveWeapon ? ActiveWeapon->GetItemData() : nullptr;
	const UNSM_WeaponComponent* WeaponComponent = ActiveWeapon ? ActiveWeapon->FindComponentByClass<UNSM_WeaponComponent>() : nullptr;

	FVector CamRelativeLocation = CameraComponent->GetRelativeLocation();
	
	if (ItemData && WeaponComponent)
	{
		const bool bAiming = WeaponComponent->IsAiming();
		if (bAiming)
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, ItemData->ZoomFOV, DeltaTime, ItemData->ZoomInSpeed);
			CamRelativeLocation = FMath::VInterpTo(CamRelativeLocation, ItemData->ZoomLocDistance, DeltaTime, ItemData->ZoomOutSpeed);

			if (ANSM_SniperRifle_Refactor* Sniper = Cast<ANSM_SniperRifle_Refactor>(ActiveWeapon))
			{
				Sniper->ShowScope();
			}
		}
		else
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ItemData->ZoomOutSpeed);
			CamRelativeLocation = FMath::VInterpTo(CamRelativeLocation, OriginalCamLocation, DeltaTime, ItemData->ZoomOutSpeed);

			
			if (ANSM_SniperRifle_Refactor* Sniper = Cast<ANSM_SniperRifle_Refactor>(ActiveWeapon))
			{
				Sniper->HideScope();
			}
		}
	}
	else
	{
		CurrentFOV = DefaultFOV;
	}

	CameraComponent->SetFieldOfView(CurrentFOV);
	CameraComponent->SetRelativeLocation(CamRelativeLocation);
}

void ANSM_CharacterBase::UpdateCommanderWidget() const
{
	if (!CommandMarkerComponent)
	{
		return;
	}

	if (!bSquadLeader)
	{
		return;
	}
	
	if (const ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{	
		if (TeamComponent)
		{
			if (TeamComponent->GetTeamTag().MatchesTagExact(PlayerController->GetTeamTag()))
			{
				if (UNSM_SquadLeaderWidget* LeaderWidget = Cast<UNSM_SquadLeaderWidget>(CommandMarkerComponent->GetWidget()))
				{
					if (bIsDead)
					{
						LeaderWidget->SetCurrentColor(FLinearColor::Gray);
					}
					else
					{						
						const bool bIsRed = TeamComponent->GetTeamTag().MatchesTagExact(FGameplayTag::RequestGameplayTag("Team.Two"));
						const FLinearColor& Color = bIsRed ? FLinearColor(1.f,0.57,0.52f,1.f) : FLinearColor(0.07f,0.9f,1.f, 1.f);				
						LeaderWidget->SetCurrentColor(Color);
					}

					if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
					{
						if (const UNSM_CommanderSubsystem* CommanderSubsystem = LocalPlayer->GetSubsystem<UNSM_CommanderSubsystem>())
						{
							LeaderWidget->SetText(FString::FromInt(CommanderSubsystem->GetLeaderCharacterIndex(this) + 1));							
						}
					}
				}
				
				CommandMarkerComponent->SetVisibility(!IsPlayerControlled());
			}
			else
			{
				CommandMarkerComponent->SetVisibility(false);
			}
		}
	}
	else
	{		
		CommandMarkerComponent->SetVisibility(false);
	}
}

float ANSM_CharacterBase::GetTurningYaw() const
{
	return TurningYaw;
}

void ANSM_CharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ANSM_CharacterBase::SwitchWeapon(const FInputActionValue &ActionValue)
{
	UNSM_EquipmentComponent *EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (IsValid(EquipmentComponent))
	{
		const float AxisValue = ActionValue.Get<float>();
		if (AxisValue > 0)
		{
			EquipmentComponent->SwitchToNextItem();
		}
		else if (AxisValue < 0)
		{
			EquipmentComponent->SwitchToPreviousItem();
		}
	}
}

void ANSM_CharacterBase::SwitchWeaponWithWheel(const FInputActionValue &ActionValue)
{
	UNSM_EquipmentComponent *EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (IsValid(EquipmentComponent))
	{
		const float AxisValue = ActionValue.Get<float>();
		if (AxisValue > 0)
		{
			EquipmentComponent->SwitchToNextItem();
		}
		else if (AxisValue < 0)
		{
			EquipmentComponent->SwitchToPreviousItem();
		}
	}
}

void ANSM_CharacterBase::DropItem(const FInputActionValue &ActionValue)
{
	if (bCanAim)
	{
		return;
	}

	UNSM_EquipmentComponent *EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->Server_DropCurrentItem();
	}
}

void ANSM_CharacterBase::Server_StartSprint_Implementation()
{
	if (bCanAim)
	{		
		//Multicast_SetMovementSpeed(200.f);
		return;
	}
	
	if (bIsCrouched)
	{
		UnCrouch();
	}

	bIsSprinting = true;
	Multicast_SetMovementSpeed(600.f);
}

void ANSM_CharacterBase::Server_StopSprint_Implementation()
{
	if (!bIsSprinting)
	{
		return;
	}
	
	bIsSprinting = false;
	Multicast_SetMovementSpeed(bCanAim ? 200.f : 400.f);
}

void ANSM_CharacterBase::RequestCrouch()
{
	Server_StopSprint();
	Crouch();
}

void ANSM_CharacterBase::RequestUncrouch()
{
	UnCrouch();
}

void ANSM_CharacterBase::RequestAimDownSight()
{	
	Server_StopSprint();
	SetCanAim(true);
	WeaponSetAiming(true);
	ServerSetMovementSpeed(200.f);
}

void ANSM_CharacterBase::RequestAimHip()
{
	SetCanAim(false);	
	WeaponSetAiming(false);	
	ServerSetMovementSpeed(400.f);
}

void ANSM_CharacterBase::WeaponSetAiming(bool bVal) const
{
	if (const UNSM_EquipmentComponent* EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>())
	{			
		if (const ANSM_WeaponBase_Refactor* ActiveWeapon = Cast<ANSM_WeaponBase_Refactor>(EquipmentComponent->GetActiveItem()))
		{
			if (UNSM_WeaponComponent* WeaponComponent = ActiveWeapon->FindComponentByClass<UNSM_WeaponComponent>())
			{
				WeaponComponent->SetIsAiming(bVal);
			}
		}
	}	
}

void ANSM_CharacterBase::NotifyHitmarker(bool bIsHeadshot /*= false*/)
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (ANSM_BaseHUD* HUD = PlayerController ? Cast<ANSM_BaseHUD>(PlayerController->GetHUD()) : nullptr)
		{
			HUD->GetHUDPackage().HitmarkerSpread = 0.f;
			HUD->GetHUDPackage().HitmarkerColor = bIsHeadshot ? FLinearColor::Red : FLinearColor::White;
		}
	}
}

float ANSM_CharacterBase::CalculateSignificance(USignificanceManager::FManagedObjectInfo* ObjectInfo,
	const FTransform& Viewpoint) const
{
	if (ObjectInfo)
	{	
		if (const ANSM_CharacterBase* CharacterBase = Cast<ANSM_CharacterBase>(ObjectInfo->GetObject()))
		{
			return FVector::Distance(CharacterBase->GetActorLocation(), Viewpoint.GetLocation());
		}
	}

	return TNumericLimits<float>::Max();
}

void ANSM_CharacterBase::PostSignificance(const USignificanceManager::FManagedObjectInfo* ObjectInfo,
	float OldSignificance, float Significance, bool bFinal) const
{
	if (ObjectInfo)
	{
		if (ANSM_CharacterBase* CharacterBase = Cast<ANSM_CharacterBase>(ObjectInfo->GetObject()))
		{
			if (UNSM_CharacterMovementComponent* CharacterMovementComponent = Cast<UNSM_CharacterMovementComponent>(GetCharacterMovement()))
			{				
				if (Significance < 10000)
				{
					//CharacterMovementComponent->SetComponentTickInterval(0.f);
				}
				else
				{
					//CharacterMovementComponent->SetComponentTickInterval(1.f);
				}
			}
		}		
	}
}

void ANSM_CharacterBase::Multicast_SetAIController_Implementation(ANSM_AIControllerBase* NewAIController)
{
	AIController = NewAIController;
}

void ANSM_CharacterBase::Server_SetAIController_Implementation(ANSM_AIControllerBase* NewAIController)
{
	Multicast_SetAIController(NewAIController);
}

void ANSM_CharacterBase::Multicast_SetCommanderWidgetVisible_Implementation(bool bVisible, bool bForce/* = false*/)
{
	return;; // Doesn't work properly
	
	// changes only allowed on local controller
	if(const ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{	
		if (TeamComponent && CommandMarkerComponent)
		{
			if (bForce || TeamComponent->GetTeamTag().MatchesTagExact(PlayerController->GetTeamTag()))
			{
				if (CommandMarkerComponent->GetWidget())
				{					
					const bool bIsRed = TeamComponent->GetTeamTag().MatchesTagExact(FGameplayTag::RequestGameplayTag("Team.Two"));
					const FLinearColor& Color = bIsRed ? FLinearColor(1.f,0.57,0.52f,1.f) : FLinearColor(0.07f,0.9f,1.f, 1.f);				
					CommandMarkerComponent->GetWidget()->SetColorAndOpacity(Color);
				}
				
				CommandMarkerComponent->SetVisibility(bVisible, true);
			}
		}
	}
}

void ANSM_CharacterBase::OnTeamAssigned(const bool bIsLeader)
{
	TArray<UMaterialInterface*> MaterialInstances;
	if (TeamComponent->GetTeamTag().GetTagName() == FName("Team.One"))
	{
		MaterialInstances = TeamOne;
	}
	else if(TeamComponent->GetTeamTag().GetTagName() == FName("Team.Two"))
	{
		MaterialInstances = TeamTwo;
	}
	else
	{
		MaterialInstances = TeamAI;
	}
	
	UNSM_EquipmentComponent* EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	if (IsValid(EquipmentComponent))
	{
		// [Vlad] - TODO: refactor weapons and equipment
		//EquipmentComponent->Server_InitializeItems();
	}
	
	if (!IsValid(GetWorld()))
	{
		return;
	}
	
	if (!IsValid(GetMesh()))
	{
		return;
	}
	
	const int32 NumMaterials = GetMesh()->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; i++)
	{
		if(!IsValid(MaterialInstances[1]))
		{
			continue;
		}
		GetMesh()->SetMaterial(i, MaterialInstances[i]);
	}
	
	if(FormationUnitComponent->CurrentSquadData.LeaderCharacter == this)
	{
		Server_SetIsSquadLeader(true);
		return;
	}
	const ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if(!IsValid(PlayerController))
	{
		return;
	}
	if(PlayerController->SquadLeaders.Contains(this))
	{
		Server_SetIsSquadLeader(true);
	}
}

void ANSM_CharacterBase::SetCanAim(const bool CanAim)
{
	bCanAim = CanAim;
	ServerSetCanAim(CanAim);
}

void ANSM_CharacterBase::Server_SetIsFiring_Implementation(const bool IsFiring)
{
	Multicast_SetIsFiring(IsFiring);
}

void ANSM_CharacterBase::Multicast_SetIsFiring_Implementation(const bool IsFiring)
{
	bIsFiring = IsFiring;
}

void ANSM_CharacterBase::HideCameraIfCharacterClose()
{
	const UNSM_EquipmentComponent *EquipmentComponent = FindComponentByClass<UNSM_EquipmentComponent>();
	bool bShouldShowMesh = true;
	bool bOwnerNoSee = false;

	if (IsValid(EquipmentComponent))
	{
		if (IsLocallyControlled() && IsPlayerControlled() && (Cam->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
		{
			bShouldShowMesh = false;
			bOwnerNoSee = true;
		}
		else if (IsValid(EquipmentComponent->GetActiveItem()))
		{
			EquipmentComponent->GetActiveItem()->GetMeshComponent()->bOwnerNoSee = false;
		}
	}

	GetMesh()->SetVisibility(bShouldShowMesh);
	if (IsValid(EquipmentComponent) && IsValid(EquipmentComponent->GetActiveItem()))
	{
		EquipmentComponent->GetActiveItem()->GetMeshComponent()->bOwnerNoSee = bOwnerNoSee;
	}
}

void ANSM_CharacterBase::ServerSetMovementSpeed_Implementation(float Speed)
{
	Multicast_SetMovementSpeed(Speed);
}

void ANSM_CharacterBase::Multicast_SetMovementSpeed_Implementation(float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
}

void ANSM_CharacterBase::Multicast_SetIsSquadLeader_Implementation(bool IsSquadLeader)
{
	bSquadLeader = IsSquadLeader;
}

void ANSM_CharacterBase::Server_SetIsSquadLeader_Implementation(bool IsSquadLeader)
{
	Multicast_SetIsSquadLeader(IsSquadLeader);
}

void ANSM_CharacterBase::Multicast_PlayAnimMontage_Implementation(UAnimMontage* Montage)
{
	NSMPlayAnimMontage(Montage);
}

void ANSM_CharacterBase::NSMPlayAnimMontage(UAnimMontage* Montage)
{
	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_Play(Montage);
}

void ANSM_CharacterBase::Server_PlayAnimMontage_Implementation(UAnimMontage *Montage)
{
	if (bIsDead)
	{
		return;
	}
	
	Multicast_PlayAnimMontage(Montage);
}

void ANSM_CharacterBase::Server_SetCharacterDataAsset_Implementation(UNSM_CharacterDataAsset* NewCharacterDataAsset)
{
	Multicast_SetCharacterDataAsset(NewCharacterDataAsset);
}

void ANSM_CharacterBase::Multicast_SetCharacterDataAsset_Implementation(UNSM_CharacterDataAsset* NewCharacterDataAsset)
{
	CharacterDataAsset = NewCharacterDataAsset;

	SetCharacterData(CharacterDataAsset->CharacterData);
}

void ANSM_CharacterBase::SetCharacterData(FNSM_CharacterData NewCharacterData)
{
	CharacterData = NewCharacterData;

	InitFromCharacterData(CharacterData);
}

void ANSM_CharacterBase::ServerSetCanAim_Implementation(const bool CanAim)
{
	bCanAim = CanAim;
}
