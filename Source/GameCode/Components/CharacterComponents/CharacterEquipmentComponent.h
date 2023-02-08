// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameCodeTypes.h"
#include "Actors/Equipment/EquipableItem.h"
#include <Subsystems/SaveSubsystem/SaveSubsystemInterface.h>
#include "CharacterEquipmentComponent.generated.h"

typedef TArray<int32, TInlineAllocator<(int32)EAmmunitionType::MAX>> TAmunitionArray;
typedef TArray<class AEquipableItem*, TInlineAllocator<(int32)EEquipmentSlots::MAX>> TItemsArray;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCurrentWeaponAmmoChanged, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChanged,const AEquipableItem*);

class ARangeWeaponItem;
class AThrowableItem;
class AMeleeWeaponItem;
class UEquipmentViewWidget;
class UWeaponWheelWidget;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UCharacterEquipmentComponent : public UActorComponent, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:
	
	UCharacterEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void BeginPlay() override;

	EEquipableItemType GetCurrentEquipperItemType() const;

	ARangeWeaponItem* GetCurrentRangeWeapon() const;

	AThrowableItem* GetCurrentThrowableWeapon() const;

	AMeleeWeaponItem* GetCurrentMeleeWeapon() const;

	bool IsEquipping() const;
	void ReloadCurrentWeapon();
	void ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo = 0, bool bChecksFull = false);

	FOnCurrentWeaponAmmoChanged OnCurrentWeaponAmmoChangedEvent;

	FOnEquippedItemChanged OnEquippedItemChanged;
	void EquipItemInSlot(EEquipmentSlots Slot);

	void AttachCurrentItemToEquippedSocket();

	void LaunchCurrentThrowableItem();

	void UnEquipCurrentItem();

	void EquipNextItem();
	void EquipPreviousItem();

	bool AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem> EquipableItemClass, int32 SlotIndex);

	void RemoveItemFromSlot(int32 SlotIndex);

	void OpenViewEquipment(APlayerController* PlayerController);
	void CloseViewEquipment();
	bool IsViewVisible() const;

	void OpenWeaponWheel(APlayerController* PlayerController);

	bool IsSelectingWeapon() const;

	void ConfirmWeaponSelection() const;

	virtual void OnLevelDeserialized_Implementation() override;

	const TArray<AEquipableItem*>& GetItems() const;


protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EAmmunitionType, int32> MaxAmunitionAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EEquipmentSlots, TSubclassOf<class AEquipableItem>> ItemsLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TSet<EEquipmentSlots> IgnoreSlotsWhileSwitching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	EEquipmentSlots AutoItemInSlot = EEquipmentSlots::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View")
	TSubclassOf<UEquipmentViewWidget> ViewWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View")
	TSubclassOf<UWeaponWheelWidget> WeaponWheelClass;

	void CreateEquipmentWidgets(APlayerController* PlayerController);

private:
	UFUNCTION(Server, Reliable)
	void Server_EquipItemInSlot(EEquipmentSlots Slot);
		
	//Создает текущее оружие
	void CreateLoadout();

	void AutoEquip();
	void EquipAnimationFinished();
	int32 NextItemsArraySlotIndex(uint32 CurrentSlotIndex);
	int32 PreviousItemsArraySlotIndex(uint32 CurrentSlotIndex);
	int32 GetAvailadleAmunitionForCurrentWeapon();
	
	bool bIsEquipping = false;

	UFUNCTION()
	void OnWeaponReloadComplete();

	UFUNCTION()
	void OnCurrentWeaponAmmoChanged(int32 Ammo);

	FDelegateHandle OnCurrentWeaponAmmoChangedHandled;
	FDelegateHandle OnCurrentWeaponReloadHandled;

	EEquipmentSlots PreviousEquippedSlot;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquippedSlot, SaveGame)
	EEquipmentSlots CurrentEquippedSlot;

	UFUNCTION()
	void OnRep_CurrentEquippedSlot(EEquipmentSlots CurrentEquipSlot_Old);

	AEquipableItem* CurrentEquippedItem;
	ARangeWeaponItem* CurrentEquippendWeapon;
	AThrowableItem* CurrentThrowableItem;
	AMeleeWeaponItem* CurrentMeleeWeapon;

	UPROPERTY(Replicated, SaveGame)
	TArray<int32> AmunitionArray;
	
	UPROPERTY(ReplicatedUsing = OnRep_ItemsArray, SaveGame)
	TArray<AEquipableItem*> ItemsArray;

	UFUNCTION()
	void OnRep_ItemsArray();

	TWeakObjectPtr<class AGCBaseCharacter> CachedBaseCharacter;
	
	FTimerHandle EquipTimer;

	UEquipmentViewWidget* ViewWidget;
	UWeaponWheelWidget* WeaponWheelWidget;
};
