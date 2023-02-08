// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GCPlayerController.generated.h"

class UPlayerHUDWidget;

UCLASS()
class GAMECODE_API AGCPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	virtual void SetPawn(APawn* InPawn) override;

	virtual void Tick(float DeltaSeconds) override;

	bool GetIgnoreCameraPitch() const;

	void SetIgnoreCameraPitch(bool bIgnoreCameraPitch_In);


protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<UPlayerHUDWidget> PlayerHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

	virtual void SetupInputComponent() override;

private:

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	
	//Crouch
	void ChangeCrouchState();

	//Prone
	void ChangeProneState();
	
	//Mantle
	void Mantle();
	
	//Jump
	void Jump();
	
	//Sprint
	void StartSprint();
	void StopSprint();

	//Swim
	void SwimForward(float Value);
	void SwimRight(float Value);
	void SwimUp(float Value);

	//Ladder
	void ClimbLadder(float Value);
	void InteractWithLadder();

	//Zipline
	void ClimbZipline(float Value);

	void InteractWithZipline();

	//Fire
	void PlayerStartFire();
	void PlayerStopFire();

	//Aim
	void StartAim();
	void StopAim();

	//Reload
	void Reload();

	//Item
	void NextItem();
	void PreviousItem();

	void EquipPrimaryItem();

	//Melee attack
	void PrimaryMeleeAttack();
	void SecondaryMeleeAttack();

	//Menu
	void ToggleMainMenu();

	//Inventory
	void Interact();
	void UseInventory();

	void ConfirmWeaponWheelSelection();

	void QuickSaveGame();
	void QuickLoadGame();

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void Debug_IncreaseGlobalDilation();
	void Debug_DecreaseGlobalDilation();
#endif

	TSoftObjectPtr<class AGCBaseCharacter> CachedBaseCharacter;

private:

	bool bIgnoreCameraPitch = false;

	void OnInteractableObjectFound(FName ActionName);
	void CreateAndInitializeWidget();

	UPlayerHUDWidget* PlayerHUDWidget = nullptr;
	UUserWidget* MainMenuWidget = nullptr;


};
