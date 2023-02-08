// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Subsystems/Streaming/StreamingSubsystemUtils.h"




APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;


	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring arm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = 1;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700;

	Team = ETeams::Player;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	UStreamingSubsystemUtils::CheckCharacterOverlapStreamingSubsystemVolume(this);

	if (IsValid(CameraCurve))
	{	
		DefaultArmLength = SpringArmComponent->TargetArmLength;
		
		FOnTimelineFloat TimelineCallBack;
		TimelineCallBack.BindUFunction(this, FName("UpdateCamera"));
		CameraSprintTimeline.AddInterpFloat(CameraCurve,TimelineCallBack);
	
	}

	if (IsValid(AimFOVCurve))
	{
		FOnTimelineFloat TimelineAimFOV;
		TimelineAimFOV.BindUFunction(this, FName("UpdateAimFOV"));
		FOVTimeline.AddInterpFloat(AimFOVCurve, TimelineAimFOV);

	}
	
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CameraSprintTimeline.TickTimeline(DeltaSeconds);

	FOVTimeline.TickTimeline(DeltaSeconds);
}

void APlayerCharacter::MoveForward(float Value)
{
	if ((GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()) &&  !FMath::IsNearlyZero(Value,1e-6f))
	//if(Value != 0)
	{
		FRotator YawRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		FVector ForwardVector = YawRotator.RotateVector(FVector::ForwardVector);
		AddMovementInput(ForwardVector, Value);
		//AddMovementInput(GetActorForwardVector(), Value);


	}
}

void APlayerCharacter::MoveRight(float Value)
{
	if ((GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()) && !FMath::IsNearlyZero(Value, 1e-6f))
	//if (Value != 0)
	{
		FRotator YawRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		FVector RightVector = YawRotator.RotateVector(FVector::RightVector);
		AddMovementInput(RightVector, Value);
		//AddMovementInput(GetActorRightVector(), Value);
	}
}

void APlayerCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
	
}

void APlayerCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void APlayerCharacter::OnStartCrouch(float HalfHeightAbjust, float ScaledHalfHeightAnjust)
{
	Super::OnStartCrouch(HalfHeightAbjust,ScaledHalfHeightAnjust);
	SpringArmComponent->TargetOffset += FVector(0.0f,0.0f, HalfHeightAbjust);

}

void APlayerCharacter::OnEndCrouch(float HalfHeightAbjust, float ScaledHalfHeightAnjust)
{
	Super::OnEndCrouch(HalfHeightAbjust, ScaledHalfHeightAnjust);
	SpringArmComponent->TargetOffset -= FVector(0.0f,0.0f,HalfHeightAbjust);
}

void APlayerCharacter::OnStartProne(float HalfHeightAbjust, float ScaledHalfHeightAnjust)
{
	Super::OnStartProne(HalfHeightAbjust, ScaledHalfHeightAnjust);

	SpringArmComponent-> TargetOffset += FVector(0.0f, 0.0f, HalfHeightAbjust + 26.0f);
}

void APlayerCharacter::OnEndProne(float HalfHeightAbjust, float ScaledHalfHeightAnjust)
{
	Super::OnEndProne(HalfHeightAbjust, ScaledHalfHeightAnjust);

	SpringArmComponent->TargetOffset += FVector(0.0f, 0.0f, HalfHeightAbjust -22.0f );

}

void APlayerCharacter::SwimForward(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator PitchYawRotator(GetControlRotation().Pitch, GetControlRotation().Yaw, 0.0f);
		FVector ForwardVector = PitchYawRotator.RotateVector(FVector::ForwardVector);
		AddMovementInput(ForwardVector, Value);
		
	}
}

void APlayerCharacter::SwimRight(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator YawRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		FVector RightVector = YawRotator.RotateVector(FVector::RightVector);
		AddMovementInput(RightVector, Value);
	
	}
}

void APlayerCharacter::SwimUp(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		AddMovementInput(FVector::UpVector, Value);
	}

}

void APlayerCharacter::OnSprintStart_Implementation()
{
	Super::OnSprintStart_Implementation();
	UE_LOG(LogTemp, Log, TEXT("AGCBaseCharacter::OnSprintStart_Implemention"));
	StartCamera();
}

void APlayerCharacter::OnSprintEnd_Implementation()
{
	Super::OnSprintEnd_Implementation();
	UE_LOG(LogTemp, Warning, TEXT("AGCBaseCharacter::OnSprintEnd_Implemention"));
	EndCamera();
}

void APlayerCharacter::UpdateCamera(float Value)
{
	SpringArmComponent->TargetArmLength = FMath::Lerp(DefaultArmLength, SprintArmLength,Value);
}

void APlayerCharacter::OnStartAimingInternal()
{
	Super::OnStartAimingInternal();

	APlayerController* PlayerController = GetController<APlayerController>();
	if (!IsValid(PlayerController))
	{
		return;	
	}

	CameraManager = PlayerController->PlayerCameraManager;

	if (IsValid(CameraManager))
	{
		ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
		
		if (IsValid(AimFOVCurve))
		{
			DefaultFOV = CameraManager->DefaultFOV;
			AimingFOV = CurrentRangeWeapon->GetAimFOV();
			FOVTimeline.Play();
			
		}
		else
		{
			CameraManager->SetFOV(CurrentRangeWeapon->GetAimFOV());
		}
		
	}

}

void APlayerCharacter::OnStopAimingInternal()
{
	Super::OnStopAimingInternal();

	APlayerController* PlayerController = GetController<APlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	CameraManager = PlayerController->PlayerCameraManager;
	
	if (IsValid(CameraManager))
	{
		ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
		
		if (IsValid(AimFOVCurve))
		{
			FOVTimeline.Reverse();

		}
		else
		{
			CameraManager->UnlockFOV();
		}
	}

}

void APlayerCharacter::UpdateAimFOV(float Value)
{
	CameraManager->SetFOV(FMath::Lerp(DefaultFOV,AimingFOV, Value));
}

