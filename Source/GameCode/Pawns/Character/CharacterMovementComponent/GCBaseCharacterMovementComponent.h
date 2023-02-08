// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../GCBaseCharacter.h"
#include <GameCode/Components/LedgeDetectorComponent.h>
#include "GCBaseCharacterMovementComponent.generated.h"

struct FMantlingMovementParameters
{
	FVector InitialLocation = FVector::ZeroVector;
	FRotator InitialRotation = FRotator::ZeroRotator;

	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;

	FVector InitialAnimationLocation = FVector::ZeroVector;

	float Duration = 1.0f;
	float StartTime = 0.0f;

	UCurveVector* MantlingCurve;

};

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
	CMOVE_None = 0 UMETA(DisplayName = "None"),
	CMOVE_Mantling UMETA(DisplayName = "Mantling"),
	CMOVE_Ladder UMETA(DisplayName = "Ladder"),
	CMOVE_Zipline UMETA(DisplayName = "Zipline"),
	CMOVE_Max UMETA(Hidden)

};

UENUM(BlueprintType)
enum class EDetachFromLadderMethod : uint8
{
	Fall = 0,
	ReachingTheTop,
	ReachingTheBottom,
	JumpOff

};

UENUM(BlueprintType)
enum class EDetachFromZiplineMethod : uint8
{
	Fall = 0,
	ReachingTheTop,
	ReachingTheBottom,
	JumpOff

};
/**
 * 
 */
UCLASS()
class GAMECODE_API UGCBaseCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	friend class FSavedMove_GC;

public:
	virtual FNetworkPredictionData_Client_Character* GetPredictionData_Client() const override;

	virtual void UpdateFromCompressedFlags(uint8 Flags);
	
	virtual void PhysicsRotation(float DeltaTime) override;

	virtual void BeginPlay() override;

	FORCEINLINE bool IsSprinting() { return bIsSprinting; }

	FORCEINLINE bool IsOutOfStamina() const { return bIsOutOfStamina; }

	void SetIsOutOfStamina(bool bIsOutOfStamina_In);
	
	virtual float GetMaxSpeed() const override;

	void StartSprint();
	void StopSprint();
	
	//Prone
	FORCEINLINE bool IsProning() const; 

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	virtual bool CanProneInCurrentState() const;

	virtual void Prone(bool bClientSimulation = false);

	virtual void UnProne(bool bClientSimulation = false);
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Character | Prone")
	bool bWantsToProne = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Character | Prone")
	bool bIsFullHeight = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Character | Prone")
	bool bProneMaintainsBaseLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement (General settings)", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float PronedHalfHeight = 34.0f;
	//

	//Mantle
	void StartMantle(const FMantlingMovementParameters& MantlingParameters);
	void EndMantle();
	bool IsMantling() const;
	//
	
	//Ladder
	void AttachToLadder(const class ALadder* Ladder);

	float GetActorToCurrentLadderProjection(const FVector& Location) const;

	float GetLadderSpeedRatio() const;

	void DetachFromLadder(EDetachFromLadderMethod DetachFromLadderMethod = EDetachFromLadderMethod::Fall);
	bool IsOnLadder() const;
	const class ALadder* GetCurrentLadder();

	//Zipline
	void AttachToZipline(const class AZipline* Zipline);
	
	float GetActorToCurrentZiplineProjection(const FVector& Location) const;

	float GetZiplineSpeedRatio() const;

	bool IsOnZipline() const;

	void DetachFromZipline(EDetachFromZiplineMethod DetachFromZiplineMethod = EDetachFromZiplineMethod::Fall);

	const class AZipline* GetCurrentZipline();

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	void PhysMantling(float DeltaTime, int32 Iterations);
	void PhysLadder(float DeltaTime, int32 Iterations);

	void PhysZipline(float DeltaTime, int32 Iterations);

	UPROPERTY(Transient, DuplicateTransient)
	AGCBaseCharacter* BaseCharacterOwner;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	//Swim
	UPROPERTY(Category = "Character Movement: Swimming", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SwimmingCapsuleRadius = 60.0f;

	UPROPERTY(Category = "Character Movement: Swimming", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SwimmingCapsuleHalfHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SprintSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float OutOfStaminaSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float OutOfProne = 100.0f;

	//Ladder
	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ClimbingOnLadderMaxSpeed = 200.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float LadderToCharacterOffset = 60.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ClimbingOnLadderBrakingDecelartion = 2048.0f;
	
	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxLadderTopOffset = 90.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MinLadderBottomOffset = 90.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float JumpOffFromLadderSpeed = 500.0f;

	//Zipline
	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ClimbingOnZiplineMaxSpeed = 200.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ZiplineToCharacterOffset = 60.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ClimbingOnZiplineBrakingDecelartion = 2048.0f;


	AGCBaseCharacter* GetBaseCharacterOwner() const;

private:
	
	bool bIsSprinting = false;
	bool bIsOutOfStamina = false;
	
	//Mantle
	FMantlingMovementParameters CurrentMantlingParameters;
	FTimerHandle MantlingTimer;
	//

	//Ladder
	 const ALadder* CurrentLadder = nullptr;

	 FRotator ForceTargetRotation = FRotator::ZeroRotator;
	 bool bForceRotation = false;

	 //Zipline
	 const AZipline* CurrentZipline = nullptr;
};

class FSavedMove_GC : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;

public:
	virtual void Clear() override;

	virtual uint8 GetCompressedFlags() const;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const override;

	virtual void SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;

	virtual void PrepMoveFor(ACharacter* Character) override;

private:
	uint8 bSavedIsSprinting : 1;
	uint8 bSavedIsMantling : 1;


};

class FNetworkPredictionData_Client_Character_GC : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:
	FNetworkPredictionData_Client_Character_GC(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;

};