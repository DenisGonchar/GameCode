// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveVector.h"
#include "../../../Actors/Interactive/Environment/Ladder.h"
#include "../../../Actors/Interactive/Environment/Zipline.h"
#include "../GCBaseCharacter.h"


FNetworkPredictionData_Client_Character* UGCBaseCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UGCBaseCharacterMovementComponent* MutableThis = const_cast<UGCBaseCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character_GC(*this);

	}
	return ClientPredictionData;
}

void UGCBaseCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	
	/*
		FLAG_Reserved_1 = 0x04,	// Reserved for future use
		FLAG_Reserved_2 = 0x08,	// Reserved for future use
		// Remaining bit masks are available for custom flags.
		FLAG_Custom_0 = 0x10, - Sprinting flag
		FLAG_Custom_1 = 0x20, - Mantling
		FLAG_Custom_2 = 0x40,
		FLAG_Custom_3 = 0x80,
	*/

	bool bWasMantling = GetBaseCharacterOwner()->bIsMantling;

	bIsSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bool bIsMantling = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;

	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (!bWasMantling && bIsMantling)
		{
			GetBaseCharacterOwner()->Mantle(true);
		}
	}

}

void UGCBaseCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (bForceRotation)
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation(); // Normalized
		CurrentRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): CurrentRotation"));

		FRotator DeltaRot = GetDeltaRotation(DeltaTime);
		DeltaRot.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): GetDeltaRotation"));

		// Accumulate a desired new rotation.
		const float AngleTolerance = 1e-3f;

		if (!CurrentRotation.Equals(ForceTargetRotation, AngleTolerance))
		{
			FRotator DesiredRotation = ForceTargetRotation;

			// PITCH
			if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, DesiredRotation.Pitch, AngleTolerance))
			{
				DesiredRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
			}

			// YAW
			if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw, AngleTolerance))
			{
				DesiredRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
			}

			// ROLL
			if (!FMath::IsNearlyEqual(CurrentRotation.Roll, DesiredRotation.Roll, AngleTolerance))
			{
				DesiredRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
			}

			// Set the new rotation.
			DesiredRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): DesiredRotation"));
			MoveUpdatedComponent(FVector::ZeroVector, DesiredRotation, /*bSweep*/ false);
		}
		else
		{	
			ForceTargetRotation = FRotator::ZeroRotator;
			bForceRotation = false;
		}
		return;
	}
	
	if (IsOnLadder())
	{
		return; 
	
	}

	if (IsOnZipline())
	{
		return;
	}

	Super::PhysicsRotation(DeltaTime);
}

void UGCBaseCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacterOwner = Cast<AGCBaseCharacter>(GetOwner());
}

float UGCBaseCharacterMovementComponent::GetMaxSpeed() const
{
	float Result = Super::GetMaxSpeed();
	if (bIsSprinting)
	{
		Result = SprintSpeed;
	}
	else if (bIsOutOfStamina)
	{
		Result = OutOfStaminaSpeed;
	}
	else if(IsProning())
	{
		Result = OutOfProne;
	}
	else if (IsOnLadder())
	{
		Result = ClimbingOnLadderMaxSpeed;
	}
	else if (IsOnZipline())
	{
		Result = ClimbingOnZiplineMaxSpeed;
	}
	else if (GetBaseCharacterOwner()->IsAiming())
	{
		Result = GetBaseCharacterOwner()->GetAimingMovementSpeed();

	}
	return Result;
}

void UGCBaseCharacterMovementComponent::StartSprint()
{
	bIsSprinting = true;
	bForceMaxAccel = 1;
}

void UGCBaseCharacterMovementComponent::StopSprint()
{
	bIsSprinting = false;
	bForceMaxAccel = 0;
}

FORCEINLINE bool UGCBaseCharacterMovementComponent::IsProning() const
{
	return BaseCharacterOwner && BaseCharacterOwner->bIsProned;
}

void UGCBaseCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (BaseCharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		const bool bIsProne = IsProning();
		if (bIsProne && (!bWantsToProne || !CanProneInCurrentState()))
		{
			UnProne(false);
		}
		else if (!bIsProne && bWantsToProne && CanProneInCurrentState())
		{
			Prone(false);
		}
	}

}

bool UGCBaseCharacterMovementComponent::CanProneInCurrentState() const
{
	return  IsMovingOnGround() && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

void UGCBaseCharacterMovementComponent::Prone(bool bClientSimulation /*= false*/)
{
	if (!HasValidData())
	{
		return;
	}

	if (!CanProneInCurrentState())
	{
		return;
	}

	//if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == PronedHalfHeight)
	if(FMath::IsNearlyEqual(BaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), PronedHalfHeight))
	{
		if (!bClientSimulation)
		{
			BaseCharacterOwner->bIsProned = true;
			bWantsToCrouch = false;
			BaseCharacterOwner->bIsCrouched = false;
		}
		BaseCharacterOwner->OnStartProne(0.f, 0.f);
		return;
	}

	// Change collision size to crouching dimensions
	const float ComponentScale = BaseCharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = BaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = BaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	// Height is not allowed to be smaller than radius.

	
	const float ClampedPronedHalfHeight = (FMath::Max3(0.f, OldUnscaledRadius, CrouchedHalfHeight))/2;
	BaseCharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedPronedHalfHeight);
	
	
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedPronedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if (!bClientSimulation)
	{
		// Crouching to a larger height? (this is rare)
		if (ClampedPronedHalfHeight > OldUnscaledHalfHeight)
		{
			FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(ProneTrace), false, CharacterOwner);
			FCollisionResponseParams ResponseParam;
			InitCollisionParams(CapsuleParams, ResponseParam);
			const bool bEncroached = GetWorld()->OverlapBlockingTestByChannel(UpdatedComponent->GetComponentLocation() - FVector(0.f, 0.f, ScaledHalfHeightAdjust), FQuat::Identity,
				UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

			// If encroached, cancel
			if (bEncroached)
			{
				BaseCharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, OldUnscaledHalfHeight);
				return;
			}
		}

		if (bProneMaintainsBaseLocation)
		{
			// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
			UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
		}

		BaseCharacterOwner->bIsProned = true;
		bWantsToCrouch = false;
		BaseCharacterOwner->bIsCrouched = false;
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	//const float MeshAdjust = ScaledHalfHeightAdjust;
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - CrouchedHalfHeight - ClampedPronedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	BaseCharacterOwner->OnStartProne(HalfHeightAdjust, ScaledHalfHeightAdjust);

	//GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Green, FString::Printf(TEXT("HalfHeightAdjust = %.2f, ScaledHalfHeightAdjust = %.2f "), HalfHeightAdjust, ScaledHalfHeightAdjust));
}

void UGCBaseCharacterMovementComponent::UnProne(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	float DesiredHeight = bIsFullHeight ? DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : CrouchedHalfHeight;

	// See if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == DesiredHeight)
	{
		if (!bClientSimulation)
		{
			BaseCharacterOwner->bIsCrouched = false;
			bWantsToCrouch = !bIsFullHeight;
			BaseCharacterOwner->bIsCrouched = !bIsFullHeight;
		}
		BaseCharacterOwner->OnEndProne(0.f, 0.f);
		return;
	}

	const float CurrentPronedHalfHeight = BaseCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float ComponentScale = BaseCharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = BaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	float HalfHeightAdjust = DesiredHeight - OldUnscaledHalfHeight;
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

	// Grow to uncrouched size.
	check(CharacterOwner->GetCapsuleComponent());

	if (!bClientSimulation)
	{
		// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		const UWorld* MyWorld = GetWorld();
		const float SweepInflation = KINDA_SMALL_NUMBER * 10.f;
		FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);

		// Compensate for the difference between current capsule size and standing size
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust); // Shrink by negative amount, so actually grow it.
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		bool bEncroached = true;

		if (!bProneMaintainsBaseLocation)
		{
			// Expand in place
			bEncroached = MyWorld->OverlapBlockingTestByChannel(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				// Try adjusting capsule position to see if we can avoid encroachment.
				if (ScaledHalfHeightAdjust > 0.f)
				{
					// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
					float PawnRadius, PawnHalfHeight;
					BaseCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
					const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
					const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
					const FVector Down = FVector(0.f, 0.f, -TraceDist);

					FHitResult Hit(1.f);
					const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
					const bool bBlockingHit = MyWorld->SweepSingleByChannel(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
					if (Hit.bStartPenetrating)
					{
						bEncroached = true;
					}
					else
					{
						// Compute where the base of the sweep ended up, and see if we can stand there
						const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
						const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + StandingCapsuleShape.Capsule.HalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
						bEncroached = MyWorld->OverlapBlockingTestByChannel(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
						if (!bEncroached)
						{
							// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
							UpdatedComponent->MoveComponent(NewLoc - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
						}
					}
				}
			}
		}
		else
		{
			// Expand while keeping base location the same.
			FVector StandingLocation = PawnLocation + FVector(0.f, 0.f, StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentPronedHalfHeight);
			bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				if (IsMovingOnGround())
				{
					// Something might be just barely overhead, try moving down closer to the floor to avoid it.
					const float MinFloorDist = KINDA_SMALL_NUMBER * 10.f;
					if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
					{
						StandingLocation.Z -= CurrentFloor.FloorDist - MinFloorDist;
						bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
					}
				}
			}

			if (!bEncroached)
			{
				// Commit the change in location.
				UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
				bForceNextFloorCheck = true;
			}
		}

		// If still encroached then abort.
		if (bEncroached)
		{
			return;
		}

		BaseCharacterOwner->bIsProned = false;
		bWantsToCrouch = !bIsFullHeight;
		BaseCharacterOwner->bIsCrouched = !bIsFullHeight;

	}
	else
	{
		bShrinkProxyCapsule = true;
	}

	// Now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
	BaseCharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DesiredHeight, true);

	HalfHeightAdjust = bIsFullHeight ? CrouchedHalfHeight - PronedHalfHeight : DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - DesiredHeight - PronedHalfHeight;

	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	BaseCharacterOwner->OnEndProne(HalfHeightAdjust, ScaledHalfHeightAdjust);

}

void UGCBaseCharacterMovementComponent::StartMantle(const FMantlingMovementParameters& MantlingParameters)
{
	CurrentMantlingParameters = MantlingParameters;
	SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Mantling);
	
}

void UGCBaseCharacterMovementComponent::EndMantle()
{
	GetBaseCharacterOwner()->bIsMantling = false;
	SetMovementMode(MOVE_Walking);
}

bool UGCBaseCharacterMovementComponent::IsMantling() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Mantling;
}

void UGCBaseCharacterMovementComponent::AttachToLadder(const class ALadder* Ladder)
{
	CurrentLadder = Ladder;
	FRotator TargetOrintationRotation = CurrentLadder->GetActorForwardVector().ToOrientationRotator();
	TargetOrintationRotation.Yaw += 180.0f;
	
	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector LadderForwardActor = CurrentLadder->GetActorForwardVector();

	float Projection = GetActorToCurrentLadderProjection(GetActorLocation());

	FVector NewCharacterLocation = CurrentLadder->GetActorLocation() + Projection * LadderUpVector + LadderToCharacterOffset * LadderForwardActor;

	if (CurrentLadder->GetIsOnTop())
	{
		NewCharacterLocation = CurrentLadder->GetAttachTopAnimMontageStartingLocation();
	}

	GetOwner()->SetActorLocation(FVector(NewCharacterLocation.X, NewCharacterLocation.Y, NewCharacterLocation.Z + 10.0f));
	GetOwner()->SetActorRotation(TargetOrintationRotation);

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Ladder);
}

float UGCBaseCharacterMovementComponent::GetActorToCurrentLadderProjection(const FVector& Location) const
{
	checkf(IsValid(CurrentLadder), TEXT("UGCBaseCharacterMovementComponent::GetCharacterToCurrentLadderProjection() cannot be invoked when current ladder is null"));

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector LadderToCharacterDistance = Location - CurrentLadder->GetActorLocation();
	return FVector::DotProduct(LadderUpVector, LadderToCharacterDistance);

}

float UGCBaseCharacterMovementComponent::GetLadderSpeedRatio() const
{
	checkf(IsValid(CurrentLadder), TEXT("UGCBaseCharacterMovementComponent::GetCharacterToCurrentLadderProjection() cannot be invoked when current ladder is null"));

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	
	return FVector::DotProduct(LadderUpVector, Velocity) / ClimbingOnLadderMaxSpeed;
}

void UGCBaseCharacterMovementComponent::DetachFromLadder(EDetachFromLadderMethod DetachFromLadderMethod /*= EDetachFromLadderMethod::Fall*/)
{	
	switch (DetachFromLadderMethod)
	{
	
		case EDetachFromLadderMethod::ReachingTheTop:
		{
			GetBaseCharacterOwner()->Mantle(true);

			break;
		}	
		case EDetachFromLadderMethod::ReachingTheBottom:
		{
			SetMovementMode(MOVE_Walking);

			break;
		}
		case EDetachFromLadderMethod::JumpOff:
		{
			FVector JumpDirection = CurrentLadder->GetActorForwardVector();

			SetMovementMode(MOVE_Falling);
			
			FVector JumpVelocity = JumpDirection * JumpOffFromLadderSpeed;

			ForceTargetRotation = JumpDirection.ToOrientationRotator();
			bForceRotation = true;
			
			Launch(JumpVelocity);

			break;
		}
		case EDetachFromLadderMethod::Fall:
		default:
		{
			SetMovementMode(MOVE_Falling);
			break;
		}
	}
}

bool UGCBaseCharacterMovementComponent::IsOnLadder() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Ladder;

}

const class AZipline* UGCBaseCharacterMovementComponent::GetCurrentZipline()
{
	return CurrentZipline;
}

const class ALadder* UGCBaseCharacterMovementComponent::GetCurrentLadder()
{
	return CurrentLadder;
}

void UGCBaseCharacterMovementComponent::AttachToZipline(const class AZipline* Zipline)
{
	CurrentZipline = Zipline;

	FRotator TargetOrintationRotation = CurrentZipline->GetActorForwardVector().ToOrientationRotator();
	TargetOrintationRotation.Yaw += -90.0f;

	FVector ZiplineUpVector = CurrentZipline->GetActorUpVector();
	FVector ZiplineForwardVector = CurrentZipline->GetActorForwardVector();
	//FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	//FVector LadderForwardActor = CurrentLadder->GetActorForwardVector();

	float Projection = GetActorToCurrentZiplineProjection(GetActorLocation());

	FVector NewCharacterLocation = CurrentZipline->GetActorLocation() + Projection * ZiplineUpVector + ZiplineToCharacterOffset * ZiplineForwardVector;
	//FVector NewCharacterLocation = CurrentZipline->GetActorLocation() + Projection * ZiplineForwardVector + ZiplineToCharacterOffset * ZiplineUpVector;

	GetOwner()->SetActorLocation(FVector(NewCharacterLocation.X, NewCharacterLocation.Y, NewCharacterLocation.Z + 10.0f));
	GetOwner()->SetActorRotation(TargetOrintationRotation);

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Zipline);
}

float UGCBaseCharacterMovementComponent::GetActorToCurrentZiplineProjection(const FVector& Location) const
{
	checkf(IsValid(CurrentZipline), TEXT("UGCBaseCharacterMovementComponent::GetCharacterToCurrentZiplineProjection() cannot be invoked when current Zipline is null"));

	FVector ZiplineUpVector = CurrentZipline->GetActorUpVector();
	FVector ZiplineToCharacterDistance = Location - CurrentZipline->GetActorLocation();
	return FVector::DotProduct(ZiplineUpVector, ZiplineToCharacterDistance);
}

float UGCBaseCharacterMovementComponent::GetZiplineSpeedRatio() const
{
	checkf(IsValid(CurrentZipline), TEXT("UGCBaseCharacterMovementComponent::GetCharacterToCurrentZiplineProjection() cannot be invoked when current Zipline is null"));

	FVector ZiplineVector = CurrentZipline->GetActorUpVector();

	return FVector::DotProduct(ZiplineVector, Velocity) / ClimbingOnZiplineMaxSpeed;
}

bool UGCBaseCharacterMovementComponent::IsOnZipline() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Zipline;
}

void UGCBaseCharacterMovementComponent::DetachFromZipline(EDetachFromZiplineMethod DetachFromZiplineMethod /*= EDetachFromZiplineMethod::Fall*/)
{
	switch (DetachFromZiplineMethod)
	{

	case EDetachFromZiplineMethod::ReachingTheTop:
	{
		//GetBaseCharacterOwner()->Mantle(true);

		break;
	}
	case EDetachFromZiplineMethod::ReachingTheBottom:
	{
		SetMovementMode(MOVE_Walking);

		break;
	}
	case EDetachFromZiplineMethod::JumpOff:
	{
		//FVector JumpDirection = CurrentLadder->GetActorForwardVector();

		SetMovementMode(MOVE_Falling);

		/*FVector JumpVelocity = JumpDirection * JumpOffFromLadderSpeed;

		ForceTargetRotation = JumpDirection.ToOrientationRotator();
		bForceRotation = true;

		Launch(JumpVelocity);*/

		break;
	}
	case EDetachFromZiplineMethod::Fall:
	default:
	{
		SetMovementMode(MOVE_Falling);
		break;
	}
	}
}

void UGCBaseCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	switch (CustomMovementMode)
	{
		case (uint8) ECustomMovementMode::CMOVE_Mantling:
		{
			PhysMantling(deltaTime, Iterations);

			break;
		}
		case (uint8)ECustomMovementMode::CMOVE_Ladder:
		{
			PhysLadder(deltaTime, Iterations);

			break;
		}
		case(uint8) ECustomMovementMode::CMOVE_Zipline:
		{
			PhysZipline(deltaTime, Iterations);
			
			break;
		}

		default:
			break;
	}


	Super::PhysCustom(deltaTime, Iterations);
}

void UGCBaseCharacterMovementComponent::PhysMantling(float DeltaTime, int32 Iterations)
{
	float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(MantlingTimer) + CurrentMantlingParameters.StartTime;

	FVector MantlingCurveValue = CurrentMantlingParameters.MantlingCurve->GetVectorValue(ElapsedTime);

	float PositionAlpha = MantlingCurveValue.X;
	float XYCorrectionAlpha = MantlingCurveValue.Y;
	float ZCorrectAlpha = MantlingCurveValue.Z;

	FVector  CurrectedInitialLocation = FMath::Lerp(CurrentMantlingParameters.InitialLocation, CurrentMantlingParameters.InitialAnimationLocation, XYCorrectionAlpha);
	CurrectedInitialLocation.Z = FMath::Lerp(CurrentMantlingParameters.InitialLocation.Z, CurrentMantlingParameters.InitialAnimationLocation.Z, ZCorrectAlpha);


	FVector NewLocation = FMath::Lerp(CurrectedInitialLocation, CurrentMantlingParameters.TargetLocation, PositionAlpha);
	FRotator NewRotation = FMath::Lerp(CurrentMantlingParameters.InitialRotation, CurrentMantlingParameters.TargetRotation, PositionAlpha);

	FVector Delta = NewLocation - GetActorLocation();
	Velocity = Delta / DeltaTime;

	FHitResult Hit;

	SafeMoveUpdatedComponent(Delta, NewRotation, false, Hit);
}

void UGCBaseCharacterMovementComponent::PhysLadder(float DeltaTime, int32 Iterations)
{
	
	CalcVelocity(DeltaTime, 1.0f, false, ClimbingOnLadderBrakingDecelartion);
	FVector Delta = Velocity * DeltaTime;

	if (HasAnimRootMotion())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), false, Hit);
		return;
	}

	FVector NewPos = GetActorLocation() + Delta;
	float  NewPosProjection = GetActorToCurrentLadderProjection(NewPos);

	if (NewPosProjection < MinLadderBottomOffset)
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheBottom);
		
		return;
	}
	else if (NewPosProjection > (CurrentLadder->GetLadderHeight() - MaxLadderTopOffset))
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheTop);
		return;
	}

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);
}

void UGCBaseCharacterMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{
	CalcVelocity(DeltaTime, 1.0f, false, ClimbingOnZiplineBrakingDecelartion);
	FVector Delta = Velocity * DeltaTime;

	/*if (HasAnimRootMotion())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), false, Hit);
		return;
	}*/

	FVector NewPos = GetActorLocation() + Delta;
	float  NewPosProjection = GetActorToCurrentZiplineProjection(NewPos);
	/*
	if (NewPosProjection < MinLadderBottomOffset)
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheBottom);

		return;
	}
	else if (NewPosProjection > (CurrentLadder->GetLadderHeight() - MaxLadderTopOffset))
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheTop);
		return;
	}
	*/

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);
}

void UGCBaseCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if (MovementMode == MOVE_Swimming)
	{
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(SwimmingCapsuleRadius, SwimmingCapsuleHalfHeight);
		
	}
	else if (PreviousMovementMode == MOVE_Swimming)
	{
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::CMOVE_Ladder)
	{
		CurrentLadder = nullptr;
	}
	
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::CMOVE_Zipline)
	{
		CurrentZipline = nullptr;
	}

	if (MovementMode == MOVE_Custom)
	{
		switch (CustomMovementMode)
		{
			case (uint8)ECustomMovementMode::CMOVE_Mantling:
			{
				
				GetWorld()->GetTimerManager().SetTimer(MantlingTimer, this, &UGCBaseCharacterMovementComponent::EndMantle, CurrentMantlingParameters.Duration, false);

			}

			default:
				break;
		}
	}


}

AGCBaseCharacter* UGCBaseCharacterMovementComponent::GetBaseCharacterOwner() const
{
	return StaticCast<AGCBaseCharacter*>(CharacterOwner);
}

void UGCBaseCharacterMovementComponent::SetIsOutOfStamina(bool bIsOutOfStamina_In)
{
	bIsOutOfStamina = bIsOutOfStamina_In;
	if (bIsOutOfStamina)
	{
		StopSprint();
		
	}
}

void FSavedMove_GC::Clear()
{
	Super::Clear();

	bSavedIsSprinting = 0;
	bSavedIsMantling = 0;

}

uint8 FSavedMove_GC::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	/*
		FLAG_Reserved_1 = 0x04,	// Reserved for future use
		FLAG_Reserved_2 = 0x08,	// Reserved for future use
		// Remaining bit masks are available for custom flags.
		FLAG_Custom_0 = 0x10, - Sprinting flag
		FLAG_Custom_1 = 0x20, - Mantling
		FLAG_Custom_2 = 0x40,
		FLAG_Custom_3 = 0x80,
	*/

	if (bSavedIsSprinting)
	{
		Result |= FLAG_Custom_0;
	}

	if (bSavedIsMantling)
	{
		Result &= ~FLAG_JumpPressed;
		Result |= FLAG_Custom_1;
	}

	return Result;
}

bool FSavedMove_GC::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_GC* NewMove = StaticCast<const FSavedMove_GC*>(NewMovePtr.Get());

	if (bSavedIsSprinting != NewMove->bSavedIsSprinting 
		|| bSavedIsMantling != NewMove->bSavedIsMantling)
	{
		return false;
	}

	return Super::CanCombineWith(NewMovePtr, InCharacter, MaxDelta);
}

void FSavedMove_GC::SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(InCharacter, InDeltaTime, NewAccel, ClientData);

	check(InCharacter->IsA<AGCBaseCharacter>());
	AGCBaseCharacter* InBaseCharacter = StaticCast<AGCBaseCharacter*>(InCharacter);
	UGCBaseCharacterMovementComponent* MovementComponent = InBaseCharacter->GetBaseCharacterMovementComponent();

	bSavedIsSprinting = MovementComponent->bIsSprinting;
	bSavedIsMantling = InBaseCharacter->bIsMantling;
}

void FSavedMove_GC::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UGCBaseCharacterMovementComponent* MovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(Character->GetMovementComponent());

	MovementComponent->bIsSprinting = bSavedIsSprinting;

}

FNetworkPredictionData_Client_Character_GC::FNetworkPredictionData_Client_Character_GC(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
	
}

FSavedMovePtr FNetworkPredictionData_Client_Character_GC::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_GC());
}
