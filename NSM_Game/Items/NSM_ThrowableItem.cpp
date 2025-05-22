#include "NordicSimMap/Items/NSM_ThrowableItem.h"
#include "NiagaraFunctionLibrary.h"
#include "NSM_ItemDataAsset.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ANSM_ThrowableItem::ANSM_ThrowableItem()
{
		
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	
	MeshComponent->SetupAttachment(CollisionComponent);
	
	CollisionComponent->OnComponentHit.AddDynamic(this, &ANSM_ThrowableItem::OnHit);
}

// ---------------------------------------------------------------------------------------------------------------------

void ANSM_ThrowableItem::BeginPlay()
{
	Super::BeginPlay();
	InitializeItem();
}

// ---------------------------------------------------------------------------------------------------------------------

void ANSM_ThrowableItem::InitializeItem() const
{
	if (ThrowableItemDA)
	{
		MeshComponent->SetStaticMesh(ThrowableItemDA->StaticMesh);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void ANSM_ThrowableItem::ItemThrown(FVector ForwardVector)
{
	ForwardVector *= ThrowableItemDA->ThrowForce;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetSimulatePhysics(true);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->BodyInstance.SetCollisionProfileName("BlockAll");
	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshComponent->AddImpulse(ForwardVector);
}

// ---------------------------------------------------------------------------------------------------------------------

void ANSM_ThrowableItem::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherActor != GetOwner()) && (OtherComp != nullptr) )
	{
		if (ItemExplosionSystem)
		{
			if (!ThrowableItemDA)
			{
				return;
			}
			if (ThrowableItemDA->ExplosionNiagaraSystem)
			{
				ItemExplosionComponet = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ThrowableItemDA->ExplosionNiagaraSystem, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
				Destroy();
			}
		}
	}

	
}



