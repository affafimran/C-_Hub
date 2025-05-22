#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NordicSimMap/UI/NSM_HUDWidget.h"
#include "NSM_BaseHUD.generated.h"


class UNSM_EndGameScreen;
class ANSM_PlayerController;
class ANSM_CharacterBase;
/**
 * 
 */

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY();

public:
	UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;
	UTexture2D* HitmarkerTopLeft;
	UTexture2D* HitmarkerTopRight;
	UTexture2D* HitmarkerBottomLeft;
	UTexture2D* HitmarkerBottomRight;
	
	float CrosshairSpread;
	float BaseCrosshairSpread;
	float HitmarkerSpread = -1.f;
	float BaseHitmarkerSpread = 20.f;
	FLinearColor CrosshairsColor = FLinearColor::White;
	FLinearColor HitmarkerColor = FLinearColor::White;

	void Reset();
	
};

UCLASS()
class NORDICSIMMAP_API ANSM_BaseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void SetHUD();

	/*
	 * Principal TPS HUD
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UNSM_BaseWidget> UserWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UNSM_RadialMenuWheelBase> RadialMenuWheelClass;

	UFUNCTION(BlueprintCallable)
	UNSM_HUDWidget* GetUserWidget() const { return UserWidget;}

	UFUNCTION(BlueprintCallable)
	UNSM_RadialMenuWheelBase* GetRadialWheel() const { return RadialMenuWheel; }
	
	UPROPERTY()
	UNSM_HUDWidget* UserWidget;

	UPROPERTY()
	UNSM_RadialMenuWheelBase* RadialMenuWheel;

	/*
	* Principal RTS HUD
	*/

	/** Bool to control the selection start state */
	UPROPERTY(Replicated)
	bool bStartSelection;

	/** Initial point to draw the selection rectangle */
	UPROPERTY(Replicated)
	FVector2D InitialSelectionPoint;
	
	/** Current selected point that updates the selection rectangle with the DrawHUD function */
	UPROPERTY(Replicated)
	FVector2D CurrentSelectionPoint;

	/** Function that gets the position of the mouse to set it to the initial and current selection point */
	UFUNCTION()
	FVector2D GetMousePositionForSelection() const;
	
	/** Reference to the NSM_PlayerController */
	UPROPERTY(Replicated)
	ANSM_PlayerController* PlayerController;
	
	UPROPERTY(Replicated)
	TArray<ANSM_CharacterBase*> ActorsSelected;

	/** Deselect all actors */
	UFUNCTION()
	void ShouldSelectValidActors() const;
	
	/** Inherit HUD function that draws the selection rectangle  */
	UFUNCTION()
	virtual void DrawHUD() override;

	void SetHUDTimerValue(float Value) const;
	
private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor, float Scale = 1.f);

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	FHUDPackage& GetHUDPackage() { return HUDPackage; }

private:
	UPROPERTY(EditAnywhere, Category = "Widget")
	TSubclassOf<UNSM_EndGameScreen> EndGameScreen;

	UPROPERTY()
	UNSM_EndGameScreen* EndGameScreenWidget = nullptr;

public:
	UNSM_EndGameScreen* GetEndGameScreen();
};
