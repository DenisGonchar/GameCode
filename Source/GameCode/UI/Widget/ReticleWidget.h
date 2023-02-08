// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameCodeTypes.h"
#include "ReticleWidget.generated.h"

class AEquipableItem;

UCLASS()
class GAMECODE_API UReticleWidget : public UUserWidget
{
	GENERATED_BODY()
public:

protected:
	UFUNCTION(BlueprintNativeEvent)
	void OnAimingStateChanged(bool bIsAiming);

	UFUNCTION(BlueprintNativeEvent)
	void OnEquippedItemChanged(const AEquipableItem* EquippedItem);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reticle")
	EReticleType CurrentReticle = EReticleType::None;

private:
	
	TWeakObjectPtr<const AEquipableItem> CurrentEquippedItem;

	void SetupCurrentReticle();
};
