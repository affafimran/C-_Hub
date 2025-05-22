#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "GameFramework/Actor.h"
#include "NSM_ThrowableItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNSM_ItemDataAsset;

UCLASS()
class NORDICSIMMAP_API ANSM_ThrowableItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANSM_ThrowableItem();
	
	/** The ItemDataAsset to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDataAsset")
	UNSM_ItemDataAsset* ThrowableItemDA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test")
	UMaterialInstance* MyMat;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USphereComponent* CollisionComponent;

	UPROPERTY(BlueprintReadOnly, Category = "VFX")
	UNiagaraComponent* ItemExplosionComponet;

	UPROPERTY(BlueprintReadOnly, Category = "VFX")
	UNiagaraSystem* ItemExplosionSystem;

	UPROPERTY(BlueprintReadOnly, Category = "Throwable Item")
	float ThrowStrength;

	UPROPERTY(BlueprintReadOnly, Category = "Throwable Item")
	float ThrowAngle;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	UFUNCTION()
	void ItemThrown(FVector ForwardVector);

	UFUNCTION()    
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void InitializeItem() const;
	
};
