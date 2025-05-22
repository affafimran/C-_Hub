#include "NSM_WeaponBase_Refactor.h"
#include "NiagaraFunctionLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/ActorChannel.h"
#include "Perception/AISense_Hearing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NordicSimMap/Character/NSM_CharacterBase.h"
#include "NordicSimMap/Components/Equipment/NSM_EquipmentComponent.h"
#include "NordicSimMap/Components/Projectile/NSM_ProjectileMovementComponent.h"
#include "NordicSimMap/Components/Weapon/NSM_WeaponComponent.h"
#include "NordicSimMap/GameFramework/NSM_PlayerController.h"
#include "NordicSimMap/Items/NSM_ItemDataAsset.h"
#include "NordicSimMap/Items/Projectiles/NSM_MainProjectile.h"

#define TRACE_LENGTH 80000.f

class ANSM_PlayerController;

ANSM_WeaponBase_Refactor::ANSM_WeaponBase_Refactor()
{	
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = SkeletalMeshComponent;
	AActor::SetReplicateMovement(true);
	
	WeaponComponent = CreateDefaultSubobject<UNSM_WeaponComponent>(TEXT("WeaponComp"));
}

void ANSM_WeaponBase_Refactor::UseLocal()
{
	Super::UseLocal();
	
	if (Holster && HasAmmo() && WeaponComponent && !WeaponComponent->IsReloading())
	{
		if (ANSM_CharacterBase* CharacterHolster = Cast<ANSM_CharacterBase>(Holster))
		{
			if(!IsValid(CharacterHolster) || CharacterHolster->bIsDead)
			{
				return;
			}
			
			//WeaponComponent->SetIsAiming(true);
			CharacterHolster->Server_StopSprint();
			WeaponComponent->SetIsFiring(true);
			CharacterHolster->SetCanAim(true);
			CharacterHolster->Server_SetIsFiring(true);

			GetWorld()->GetTimerManager().ClearTimer(ShotTimerHandle);
			
			if (!WeaponComponent->IsAiming())
			{
				GetWorld()->GetTimerManager().SetTimer(
					ShotTimerHandle,
					this,
					&ANSM_WeaponBase_Refactor::OnShotTimerExpired,
					3.f,
					false);
			}

			FTransform ShotLocationSocketTransform{FTransform::Identity};
			if (!GetShotLocationSocketTransform(ShotLocationSocketTransform))
			{
				return;
			}
			
			FVector StartLocation = ShotLocationSocketTransform.GetTranslation();
			FVector EndLocation;
			bool MissShot = false;
			FCollisionQueryParams CollisionParams;
			CollisionParams.AddIgnoredActor(CharacterHolster);
			CollisionParams.AddIgnoredActor(this);
			FHitResult HitResult;

			// Broadcast sound event for AI perception
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), StartLocation, 1.0f, CharacterHolster, 0, FName("WeaponFire"));
			
			if (CharacterHolster->IsPlayerControlled())
			{
				GetHitFromPlayer(HitResult,EndLocation);
			}
			else
			{
				GetHitFromAI(EndLocation, MissShot, StartLocation, CharacterHolster);
				HitResult.ImpactPoint = EndLocation;
			}
			
			Server_Fire(HitResult.ImpactPoint, MissShot);
		}
	}
}

bool ANSM_WeaponBase_Refactor::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	WroteSomething |= Channel->ReplicateSubobject(WeaponComponent, *Bunch, *RepFlags);
	return WroteSomething;
}

void ANSM_WeaponBase_Refactor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSM_WeaponBase_Refactor, Holster);	
	DOREPLIFETIME(ANSM_WeaponBase_Refactor, RandomStream);
}

void ANSM_WeaponBase_Refactor::ConsumeAmmo()
{
	if (!IsValid(WeaponComponent))
		return;

	const int32 CurrentBullets = WeaponComponent->GetCurrentBulletsInMagazine();
	WeaponComponent->SetCurrentBullets(CurrentBullets - 1);
}

void ANSM_WeaponBase_Refactor::WeaponShotRecoil()
{
	ANSM_CharacterBase* WeaponHolster = Cast<ANSM_CharacterBase>(GetHolster());
	if (IsValid(WeaponHolster) && GetItemData())
	{
		WeaponHolster->SetShotSpread(
			FMath::Min(GetItemData()->ShotSpreadFactor + WeaponHolster->GetShotSpread(), GetItemData()->MaxShootingSpread));
		
		if (ANSM_PlayerController* PlayerController = Cast<ANSM_PlayerController>(WeaponHolster->GetController()))
		{
			const float RandomPitchValue = FMath::FRandRange(-GetItemData()->Recoil, 0.f);
			const float RandomYawValue = FMath::FRandRange(-GetItemData()->Recoil, GetItemData()->Recoil);
			PlayerController->AddPitchInput(RandomPitchValue);
			PlayerController->AddYawInput(RandomYawValue);
		}
	}
}

void ANSM_WeaponBase_Refactor::WeaponShotCamShake()
{
	if (CameraShakeClass)
	{
		ULegacyCameraShake* CameraShakeInstance = NewObject<ULegacyCameraShake>(this, CameraShakeClass);
		if (GetItemData())
		{
			CameraShakeInstance->OscillationDuration = GetItemData()->CamShakeOscillationDuration;
			CameraShakeInstance->RotOscillation = GetItemData()->CamShakeRotOscillation;
			CameraShakeInstance->LocOscillation = GetItemData()->CamShakeLocOscillation;
		}
	}
}

void ANSM_WeaponBase_Refactor::InitializeItem()
{
	Super::InitializeItem();

	if (!SkeletalMeshComponent || !ItemData)
	{
		return;
	}

	InitializeMesh();
	SkeletalMeshComponent->SetAnimInstanceClass(ItemData->WeaponAnimClass);
	if (WeaponComponent)
	{
		WeaponComponent->Initialize();
	}

	ProjectileClass = ItemData->WeaponProjectile;
}

void ANSM_WeaponBase_Refactor::Server_RequestReload_Implementation()
{
	if (!IsValid(WeaponComponent) || !IsValid(ItemData) || !IsValid(Holster))
	{
		return;
	}

	if (WeaponComponent->IsReloading() || WeaponComponent->GetCurrentBulletsInMagazine() >= ItemData->MagazineSize)
	{
		return;
	}
	
	WeaponComponent->SetIsReloading(true);
	
	if (ANSM_CharacterBase* CharacterHolster = Cast<ANSM_CharacterBase>(Holster))
	{			
		if (IsValid(ItemData->ReloadAnimation))
		{
			CharacterHolster->Server_PlayAnimMontage(ItemData->ReloadAnimation);
		}
		if(IsValid(ItemData->WeaponReloadAnimation))
		{
			Server_PlayAnimMontage(ItemData->WeaponReloadAnimation);
		}
	}
}

void ANSM_WeaponBase_Refactor::Server_InitializeItem_Implementation()
{
	Multicast_InitializeItem();
}

void ANSM_WeaponBase_Refactor::Multicast_InitializeItem_Implementation()
{
	InitializeItem();
	Server_InitializeMesh();
}

bool ANSM_WeaponBase_Refactor::HasAmmo()
{	
	if (!IsValid(GetHolster()) || !IsValid(WeaponComponent))
	{
		return false;
	}
	return WeaponComponent->GetCurrentBulletsInMagazine() > 0;
}

bool ANSM_WeaponBase_Refactor::GetHitFromPlayer(FHitResult& HitResult, FVector& EndLocation)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return false;
	}
	
	FVector2d ViewportSize; 
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2d& CrosshairLocation(ViewportSize * 0.5f);

	FVector CrosshairWorldPosition, CrosshairWorldDirection;
	const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
									UGameplayStatics::GetPlayerController(this, 0),
									CrosshairLocation,
									CrosshairWorldPosition,
									CrosshairWorldDirection);


	if (bScreenToWorld)
	{
		const FVector& Start = CrosshairWorldPosition;
		const FVector& End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		QueryParams.AddIgnoredActor(GetOwner());

		if (const UNSM_EquipmentComponent* EquipmentComponent = GetOwner()->FindComponentByClass<UNSM_EquipmentComponent>())
		{
			for (const ANSM_ItemBase* Item : EquipmentComponent->GetItemsActors())
			{				
				QueryParams.AddIgnoredActor(Item);
			}
		}
       
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility,
			QueryParams);


		if (!HitResult.bBlockingHit)
		{
			HitResult.ImpactPoint = End;
		}
	}

	return true;
}

void ANSM_WeaponBase_Refactor::GetHitFromAI(FVector& EndLocation, bool& MissShot, FVector StartLocation,
	ANSM_CharacterBase* WeaponHolster)
{
	const AAIController* Controller = UAIBlueprintHelperLibrary::GetAIController(WeaponHolster);
	const AActor* FocusActor = Controller ? Controller->GetFocusActor() : nullptr;
	if (IsValid(FocusActor))
	{
		FVector Direction = FocusActor->GetActorLocation() - StartLocation;
		Direction.Normalize();
		EndLocation = StartLocation + Direction * ItemData->AttackRange;

		// Do a range check
		if ((EndLocation - StartLocation).SquaredLength() < FMath::Square(ItemData->AttackRange))
		{
			FHitResult HitResult;
			if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility))
			{
				if (HitResult.GetActor() == FocusActor)
				{
					// We'll hit the target
					return;
				}
			}

			// Check Head
			USkeletalMeshComponent* SKMComp = FocusActor->FindComponentByClass<USkeletalMeshComponent>();
			UWorld* World = GetWorld();
			
			auto BoneTraceLambda = [SKMComp, World, StartLocation, FocusActor](const FName& BoneName, FVector& outLocation) -> bool
			{
				outLocation = SKMComp->GetBoneLocation(BoneName, EBoneSpaces::WorldSpace);
				FHitResult HitResult;
				
				if (World->LineTraceSingleByChannel(HitResult, StartLocation, outLocation, ECC_Visibility))
				{
					return HitResult.GetActor() == FocusActor;
				}

				return false;
			};

			FVector HitLocation = FVector::ZeroVector;

			// Check Head
			if (BoneTraceLambda("Head01", HitLocation))
			{
				EndLocation = HitLocation;
				return;
			}

			// Check Shoulders
			if (BoneTraceLambda("L_UpperArm", HitLocation))
			{
				EndLocation = HitLocation;
				return;
			}
			
			if (BoneTraceLambda("R_UpperArm", HitLocation))
			{
				EndLocation = HitLocation;
				return;
			}

			// Check Legs
			if (BoneTraceLambda("R_Thigh", HitLocation))
			{
				EndLocation = HitLocation;
				return;
			}			
			
			if (BoneTraceLambda("L_Thigh", HitLocation))
			{
				EndLocation = HitLocation;
				return;
			}
		}
	}
	else if (FVector::ZeroVector != TargetShot)
	{
		FVector Direction = TargetShot - StartLocation;
		Direction.Normalize();
		EndLocation = StartLocation + Direction * ItemData->AttackRange;
	}
	else
	{
		EndLocation = StartLocation + WeaponHolster->GetActorForwardVector() * ItemData->AttackRange;
	}
}

void ANSM_WeaponBase_Refactor::OnShotTimerExpired()
{	
	GetWorld()->GetTimerManager().ClearTimer(ShotTimerHandle);
	if (WeaponComponent)
	{
		if (WeaponComponent->IsAiming())
		{
			return;
		}
	}
	
	if (ANSM_CharacterBase* CharacterBase = Cast<ANSM_CharacterBase>(Holster))
	{
		CharacterBase->RequestAimHip();
	}
}

void ANSM_WeaponBase_Refactor::Multicast_Fire_Implementation(const FVector& TraceHitTarget, bool MissedShot)
{
	OnFire(TraceHitTarget, MissedShot);	
}

void ANSM_WeaponBase_Refactor::Server_Fire_Implementation(const FVector& TraceHitTarget, bool MissedShot)
{
	Multicast_Fire(TraceHitTarget, MissedShot);
}

void ANSM_WeaponBase_Refactor::Notify_OnReloadComplete()
{
	if (!HasAuthority() || !IsValid(ItemData) || !IsValid(WeaponComponent))
	{
		return;
	}

	const int32 MagazineSize = ItemData->MagazineSize;
	const int32 BulletsInMagazine = WeaponComponent->GetCurrentBulletsInMagazine();	
	const int32 CurrentAmmo = WeaponComponent->GetCurrentAmmo();	
	const int32 AmmoToAdd = FMath::Clamp(MagazineSize - BulletsInMagazine, 0, CurrentAmmo);
	
	WeaponComponent->SetCurrentBullets(BulletsInMagazine + AmmoToAdd);
	WeaponComponent->SetIsReloading(false);
}

FVector ANSM_WeaponBase_Refactor::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const
{	
	const FVector& ShotDir = (HitTarget - TraceStart).GetSafeNormal();
	const float Angle = WeaponSpread;
    const FVector& Adjusted = RandomStream.VRandCone(ShotDir, FMath::DegreesToRadians(RandomStream.FRandRange(0.f ,  Angle)));

	if (ItemData->bDebugSpread)
	{
		DrawDebugCone(GetWorld(), TraceStart, ShotDir, (HitTarget - TraceStart).Size(), FMath::DegreesToRadians(Angle), FMath::DegreesToRadians(Angle), 12, FColor::Green, true);
		DrawDebugLine(GetWorld(), TraceStart, TraceStart + Adjusted * (HitTarget - TraceStart).Size(), FColor::Red, true);
	}
	
	return Adjusted;	
}

bool ANSM_WeaponBase_Refactor::GetShotLocationSocketTransform(FTransform& outTransform) const
{
	if (const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName(GetItemData()->MuzzleSocket))
	{
		outTransform = MuzzleFlashSocket->GetSocketTransform(Cast<USkeletalMeshComponent>(GetMeshComponent()));
		return true;
	}

	return false;
}

void ANSM_WeaponBase_Refactor::PlayFireAnimation()
{
	if(IsValid(ItemData->WeaponUsableAnimation))
	{
		if(IsValid(SkeletalMeshComponent) && IsValid(SkeletalMeshComponent->GetAnimInstance()))
		{
			if(UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance())
			{
				AnimInstance->Montage_Play(ItemData->WeaponUsableAnimation);
			}
		}
	}
}

void ANSM_WeaponBase_Refactor::OnFire(const FVector& TraceHitTarget, bool MissedShot)
{
	if (!HasAmmo())
	{
		return;
	}	
	FTransform SocketTransform;
	if (GetShotLocationSocketTransform(SocketTransform))
	{
		// From muzzle flash socket to hit location from trace under crosshairs
		const FRotator& TargetRotation = TraceEndWithScatter(SocketTransform.GetLocation(), TraceHitTarget).Rotation();
		SocketTransform.SetRotation(TargetRotation.Quaternion());

		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Cast<APawn>(GetOwner());			
			
			UWorld* World = GetWorld();
			if (World)
			{
				ANSM_MainProjectile* Proj = World->SpawnActorDeferred<ANSM_MainProjectile>(
					ProjectileClass,
					SocketTransform,
					SpawnParams.Owner,
					SpawnParams.Instigator);

				if (Proj)
				{
					Proj->FireCauser = GetHolster();
					Proj->SetLifeSpan(ItemData->AttackRange / Proj->ProjectileMovementComponent->MaxSpeed);
					Proj->ProjectileMovementComponent->AddIgnoredActor(this);
					Proj->ProjectileMovementComponent->AddIgnoredActor(GetOwner());
					Proj->MissedShot = MissedShot;
					
					if (const UNSM_EquipmentComponent* EquipmentComponent = GetOwner()->FindComponentByClass<UNSM_EquipmentComponent>())
					{
						for (const ANSM_ItemBase* Item : EquipmentComponent->GetItemsActors())
						{				
							Proj->ProjectileMovementComponent->AddIgnoredActor(Item);
						}
					}

					Proj->FinishSpawning(SocketTransform);
				}

				if (ItemData)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ItemData->CascadeMuzzleFlash.ParticleSystem,
						SocketTransform.GetLocation(),
						SocketTransform.GetRotation().Rotator(),
						ItemData->CascadeMuzzleFlash.Scale);

					
					ANSM_CharacterBase* CharacterHolster = Cast<ANSM_CharacterBase>(GetHolster());
					if(IsValid(ItemData->UsableAnimation) && CharacterHolster)
					{
						CharacterHolster->PlayAnimMontage(ItemData->UsableAnimation);
					}
	
					PlayFireAnimation();
				}

				if (HasAuthority())
				{					
					ConsumeAmmo();
				}
				
				WeaponShotRecoil();
				WeaponShotCamShake();
			}
		}
	}	
}
