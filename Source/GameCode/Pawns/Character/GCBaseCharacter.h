// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameCodeTypes.h"
#include "GenericTeamAgentInterface.h"
#include <UObject/ScriptInterface.h>
#include <Subsystems/SaveSubsystem/SaveSubsystemInterface.h>
#include "SignificanceManager.h"
#include "GCBaseCharacter.generated.h"

class IInteractable;
class UInventoryItem;

USTRUCT(BlueprintType)
struct FMantlingSetting
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* MantlingMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* FPMantlingMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UCurveVector* MantlingCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AnimationCorrectionXY = 65.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AnimationCorrectionZ = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinHeight = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHeightStartTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinHeightStartTime = 0.5f;


};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAimingStateChanged, bool)
DECLARE_DELEGATE_OneParam(FOnInteractableObjectFound, FName)

class AInteractiveActor;
class UGCBaseCharacterMovementComponent;
class UCharacterEquipmentComponent;
class UCharacterAttributeComponent;
class UCharacterInventoryComponent;
class UWidgetComponent;

UCLASS(Abstract,NotBlueprintable)
class GAMECODE_API AGCBaseCharacter : public ACharacter, public IGenericTeamAgentInterface, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:
	AGCBaseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	//@ ISaveSubsystemInterface
	virtual void OnLevelDeserialized_Implementation() override;

	//@ ~ISaveSubsystemInterface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void Tick(float DeltaTime) override;

	virtual void MoveForward(float Value) {};
	virtual void MoveRight(float Value) {};
	virtual void Turn(float Value) {};
	virtual void LookUp(float Value) {};
	
	//Crouch
	virtual void ChangeCrouchState();

	//Prone
	virtual void ChangeProneState();
	
	virtual void Prone(bool bClientSimulation = false);

	virtual void UnProne(bool bIsFullHeight, bool bClientSimulation = false);

	virtual bool CanProne() const;

	virtual void OnStartProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);
	virtual void OnEndProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnStartProne", ScriptName = "OnStartProne"))
	void K2_OnStartProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEndProne", ScriptName = "OnEndProne"))
	void K2_OnEndProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	UPROPERTY(BlueprintReadOnly, Category = "Character | Prone")
	bool bIsProned = false;

	FVector BaseTranslationOffset = FVector::ZeroVector;
	
	//Jump
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	//Fall
	virtual void Falling() override;
	virtual void NotifyJumpApex() override;
	virtual void Landed(const FHitResult& Hit) override;

	//Sprint
	virtual void StartSprint();
	virtual void StopSprint();

	//Swim
	virtual void SwimForward(float Value) {};

	virtual void SwimRight(float Value) {};

	virtual void SwimUp(float Value) {};

	//Mantle
	UFUNCTION(BlueprintCallable)
	void Mantle(bool bForce = false);

	UPROPERTY(ReplicatedUsing = OnRep_IsMantlong)
	bool bIsMantling;
	
	UFUNCTION()
	void OnRep_IsMantlong(bool bWasMantling);

	//Fire
	void StartFire();
	void StopFire();

	//Fire | Aim
	FOnAimingStateChanged OnAimingStateChanged;
	
	FRotator GetAimOffset();
	void StartAiming();
	void StopAiming();
	bool IsAiming() const;
	float GetAimingMovementSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
	void OnStartAiming();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
	void OnStopAiming();

	//Reload
	void Reload();

	void NextItem();
	void PreviousItem();

	void EquipPrimaryItem();

	FORCEINLINE UGCBaseCharacterMovementComponent* GetBaseCharacterMovementComponent() const {return GCBaseCharacterMovementComponent; }

	const UCharacterEquipmentComponent* GetCharacterEquipmentComponent() const;
	UCharacterEquipmentComponent* GetCharacterEquipmentComponent_Muteble() const;
	const UCharacterAttributeComponent* GetCharacterAttributeComponent() const;

	UCharacterAttributeComponent* GetCharacterAttributeComponent_Muteble() const;

	// IK Socket
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetIKRightFootSocketOffset () const {return IKRightFootSocketOffset;}

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetIKLeftFootSocketOffset() const { return IKLeftFootSocketOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetIKPelvisSocketOffset() const { return IKPelvisOffset; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | IK Setting", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float IKTraceExtendDistance = 30.0f;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character | IK Setting", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float IKInterpSpeed = 20.0f;
	//

	void RegisterInteractiveActor(AInteractiveActor* InteractiveActor);
	void UnregisterInteractiveActor(AInteractiveActor* InteractiveActor);


	//Ladder
	void ClimbLadderUp(float Value);
	void InteractWithLadder();
	const class ALadder* GetAvailableLadder() const;

	//Zipline
	void ClimbZipline(float Value);
	void InteractWithZipline();
	const class AZipline* GetAvailableZipline() const;

	void Stamina(bool bStamina);

	UFUNCTION(BlueprintCallable)
	bool IsSwimmingUnderWater() const;

	void PrimaryMeleeAttack();
	void SecondaryMeleeAttack();

	void Interact();

	bool PickupItem(TWeakObjectPtr<UInventoryItem> ItemToPickup);
	void UseInventory(APlayerController* PlayerController);


	UPROPERTY(VisibleDefaultsOnly, Category = "Character | Components")
	UWidgetComponent* HealthBarProgressComponent;

	void InitializeHealthProgress();

	FOnInteractableObjectFound OnInteractableObjectFound;

/** IGenericTeamAgentInterface */
	virtual FGenericTeamId GetGenericTeamId() const override;

/** ~IGenericTeamAgentInterface */
	
	void ConfirmWeaponSelection();

protected:
	
	//Sprint
	UFUNCTION(BlueprintNativeEvent, Category = "Character | Movement")
	void OnSprintStart();
	virtual void OnSprintStart_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Character | Movement")
	void OnSprintEnd();
	virtual void OnSprintEnd_Implementation();

	virtual bool CanSprint();

	bool CanMantle() const;

	virtual void OnMantle(const FMantlingSetting& MantlingSetting, float MantlingAnimationStartTime);


	UGCBaseCharacterMovementComponent* GCBaseCharacterMovementComponent;

	// IK Name socket
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK Socket name")
	FName RightFootSocketName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK Socket name")
	FName LeftFootSocketName;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Movement")
	class ULedgeDetectorComponent* LedgeDetertorComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSetting HighMantleSettings;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSetting LowMantleSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling", meta = (ClampMix = 0.0f, UIMin = 0.0f))
	float LowMantleMaxHeight = 125.0f;

	//Attribute
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterAttributeComponent* CharacterAttributeComponent;

	virtual void OnDeath();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,  Category = "Character | Animations")
	class UAnimMontage* OnDeathAnimMontage;

	//Fall
	//Damage depending from fall height (in meters)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Attributes")
	class UCurveFloat* FallDamageCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Oxygen")
	FName SocketHead;

	//Weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterEquipmentComponent* CharacterEquipmentComponent;

	virtual void OnStartAimingInternal();
	virtual void OnStopAimingInternal();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Team")
	ETeams Team = ETeams::Enemy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Team")
	float LineOfSightDistance = 500.0f;

	void TraceLineOfSight();

	UPROPERTY()
	TScriptInterface<IInteractable> LineOfSightObject;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterInventoryComponent* CharacterInventoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	bool bIsSignificanceEnabled = true; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float VeryHighSignificanceDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float HighSignificanceDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float MediumSignificanceDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float LowSignificanceDistance = 6000.0f;

private:

	float SingnificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint);
	void PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal);

	//Fall
	FVector CurrentFallApex;


	//
	void UpdateIkSetting(float DeltaSeconds);
	float GetIKOffsetForSocket(const FName& SocketName) const;
	float CalculateIKPelvisOffset();

	float IKRightFootSocketOffset = 0.0f;
	float IKLeftFootSocketOffset = 0.0f;
	float IKPelvisOffset = 0.0f;

	float IKTraceDistance = 0.0f;
	float IKScale = 0.0f;
	//

	//Sprint
	void TryChangeSprintState(float DeltaTime);
	bool bIsSprintRequested = false;
	
	//Aim
	bool bIsAiming;
	float CurrentAimingMovementSpeed;

	//Mantle
	const FMantlingSetting& GetMantlingSetting(float LedgeHeight) const;
	TArray<AInteractiveActor*, TInlineAllocator<10>> AvaibleInteractiveActors;
	
	void EnableRagdoll();

};
