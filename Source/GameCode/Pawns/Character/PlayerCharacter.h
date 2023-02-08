// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GCBaseCharacter.h"
#include "Components/TimelineComponent.h"
#include <Curves/CurveFloat.h>
#include "PlayerCharacter.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GAMECODE_API APlayerCharacter : public AGCBaseCharacter
{
	GENERATED_BODY()
	
public:
	
	APlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void MoveForward(float Value) override;
	virtual void MoveRight(float Value) override;
	virtual void Turn(float Value) override;
	virtual void LookUp(float Value) override;

	//Camera crouch
	virtual void OnStartCrouch(float HalfHeightAbjust, float ScaledHalfHeightAnjust) override;
	virtual void OnEndCrouch(float HalfHeightAbjust, float ScaledHalfHeightAnjust) override;
	
	//Prone
	virtual void OnStartProne(float HalfHeightAbjust, float ScaledHalfHeightAnjust) override;
	virtual void OnEndProne(float HalfHeightAbjust, float ScaledHalfHeightAnjust) override;

	//Sprint
	virtual void OnSprintStart_Implementation() override;
	virtual void OnSprintEnd_Implementation() override;

	//Swim
	virtual void SwimForward(float Value) override;
	virtual void SwimRight(float Value) override;
	virtual void SwimUp(float Value) override;

	//
	FORCEINLINE void StartCamera() { CameraSprintTimeline.Play(); };
	UFUNCTION()
	void UpdateCamera(float Value);
	FORCEINLINE void EndCamera() { CameraSprintTimeline.Reverse(); };
	//

protected:

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "Character | Camera")
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | SpringArm")
	class USpringArmComponent* SpringArmComponent;

	//
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Curve")
	UCurveFloat* CameraCurve;

	FTimeline CameraSprintTimeline;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float SprintArmLength = 600.0f;

	float DefaultArmLength = 0.0f;
	//


	//Aim
	virtual void OnStartAimingInternal() override;
	virtual void OnStopAimingInternal() override;

	UFUNCTION()
	void UpdateAimFOV(float Value);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Curve")
	UCurveFloat* AimFOVCurve;
	
	FTimeline FOVTimeline;
	
	class APlayerCameraManager* CameraManager;
		
	float DefaultFOV = 0.0f;
	float AimingFOV = 0.0f;


};
