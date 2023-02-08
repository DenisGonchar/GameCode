// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerCharacter.h"
#include "FPPlayerCharacter.generated.h"


UCLASS()
class GAMECODE_API AFPPlayerCharacter : public APlayerCharacter
{
	GENERATED_BODY()
	
public:
	AFPPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PossessedBy(AController* NewController) override;

	virtual void Tick(float DeltaTime) override;

	virtual void OnStartCrouch(float HalfHeightAbjust, float ScaledHalfHeightAnjust) override;

	virtual void OnEndCrouch(float HalfHeightAbjust, float ScaledHalfHeightAnjust) override;

	virtual FRotator GetViewRotation() const override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | First person")
	class USkeletalMeshComponent*  FirstPersonMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | First person")
	class UCameraComponent* FirstPersonCameraComponent;

	virtual void OnMantle(const FMantlingSetting& MantlingSetting, float MantlingAnimationStartTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | First person | Camera | Ladder | Pitch", meta = (UIMin = -89.0f, UIMax = 89.0f) )
	float LadderCameraMinPitch = -60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | First person | Camera | Ladder | Pitch", meta = (UIMin = -89.0f, UIMax = 89.0f))
	float LadderCameraMaxPitch = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | First person | Camera | Ladder | Yaw", meta = (UIMin = 0.0f, UIMax = 359.0f))
	float LadderCameraMinYaw = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | First person | Camera | Ladder | Yaw", meta = (UIMin = 0.0f, UIMax = 359.0f))
	float LadderCameraMaxYaw = 175.0f;

private:
	
	void OnLadderStarted();

	void OnLadderStopped();
	
	FTimerHandle FPMontageTimer;
	
	bool IsFPMontagePlaying() const;

	void OnFPMontageTimerElapsed();

	TWeakObjectPtr<class AGCPlayerController> GCPlayerController;

};
