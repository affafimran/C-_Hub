#pragma once

#include "CoreMinimal.h"
#include "NSM_SquadActionDataAsset.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"
#include "Interactable/NSM_InteractableActorBase.h"
#include "NSM_ItemBase.generated.h"

class USphereComponent;

UCLASS()
class NORDICSIMMAP_API ANSM_ItemBase : public ANSM_InteractableActorBase
{
	GENERATED_BODY() 
	
public:	
	// Sets default values for this actor's properties
	ANSM_ItemBase();

	virtual bool IsSupportedForNetworking() const override{ return true;}
	virtual void BeginPlay() override;

	virtual void UseLocal();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetHolster(AActor* NewHolster);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetHolster(AActor* NewHolster);


	UFUNCTION(Server, Reliable)
	void Server_PlayAnimMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAnimMontage(UAnimMontage* Montage);
	
    UFUNCTION(BlueprintCallable, Category = "Item")
    AActor* GetHolster() const {return Holster;}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseItem(FGameplayTag AbilityTag, AActor* SourceActor);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UseItem(FGameplayTag AbilityTag, AActor* SourceActor);

	UFUNCTION(Server, Reliable)
	virtual void Server_PickUp(const bool bIsOverlapping, AActor* PickedUpBy);

    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetMaxQuantity() const;

    UFUNCTION(BlueprintCallable, Category = "Item")
	UNSM_ItemDataAsset* GetItemData() const;

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetItemState(ENSM_ItemState NewItemState);

	UPROPERTY(ReplicatedUsing = OnRep_ItemState, VisibleAnywhere)
	ENSM_ItemState ItemState;

	UFUNCTION()
	virtual void OnRep_ItemState();
	
	UFUNCTION(BlueprintCallable)
	UNSM_AbilitySystemComponent* GetAbilitySystemComponent() const {return AbilitySystemComponent;}
	
	UFUNCTION(BlueprintCallable, Category = "Item")
	void StartCooldown();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void EndCooldown();

	virtual void OnItemDataChanged() override;

	virtual void InitializeItem() override;
	virtual void Interact_Implementation(AActor* ActorInteracting) override;
	
	UFUNCTION()
	virtual void OnPickUpOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	virtual void Tick(float DeltaTime) override;

protected:
	void AddStartupGameplayAbilities();
	
	UFUNCTION()
	void UpdateCooldown(float DeltaTime);
	
public:
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Item")
	AActor* Holster;

	UPROPERTY()
	uint8 bAbilitiesInitialized : 1;
	
	TObjectPtr<UNSM_AbilitySystemComponent> AbilitySystemComponent;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	bool bIsInCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float CooldownRemaining;
};
