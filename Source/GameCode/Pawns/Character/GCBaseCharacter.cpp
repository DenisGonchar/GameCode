// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacter.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "Curves/CurveVector.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CharacterMovementComponent/GCBaseCharacterMovementComponent.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"
#include <Components/CharacterComponents/CharacterInventoryComponent.h>
#include "Components/LedgeDetectorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Actors/Interactive/InteractiveActor.h"
#include "Actors/Interactive/Environment/Ladder.h"
#include "Actors/Interactive/Environment/zipline.h"
#include "Actors/Interactive/Interface/Interactive.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Actors/Equipment/Weapons/MeleeWeaponItem.h"
#include "UI/Widget/World/GCAttributeProgressBar.h"
#include "Inventory/Items/InventoryItem.h"
#include "GameCodeTypes.h"
#include "SignificanceManager.h"

AGCBaseCharacter::AGCBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGCBaseCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	GCBaseCharacterMovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(GetCharacterMovement());

	//
	IKScale = GetActorScale3D().Z;
	IKTraceDistance = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * IKScale;
	
	BaseTranslationOffset = GetMesh()->GetRelativeLocation();

	LedgeDetertorComponent = CreateDefaultSubobject<ULedgeDetectorComponent>(TEXT("LedgeDetertor"));

	GetMesh()->CastShadow = true;
	GetMesh()->bCastDynamicShadow = true;

	AvaibleInteractiveActors.Reserve(10);
	CharacterAttributeComponent = CreateDefaultSubobject<UCharacterAttributeComponent>(TEXT("CharacterAttribute"));
	CharacterEquipmentComponent = CreateDefaultSubobject<UCharacterEquipmentComponent>(TEXT("CharacterEquipment"));
	CharacterInventoryComponent = CreateDefaultSubobject<UCharacterInventoryComponent>(TEXT("InventoryComponent"));

	HealthBarProgressComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarProgressComponent"));
	HealthBarProgressComponent->SetupAttachment(GetCapsuleComponent());

}

void AGCBaseCharacter::BeginPlay()
{
	Super::BeginPlay();


	CharacterAttributeComponent->OnDeathEvent.AddUObject(this, &AGCBaseCharacter::OnDeath);
	CharacterAttributeComponent->OutOfStaminaEventSignature.AddUObject(this, &AGCBaseCharacter::Stamina);

	InitializeHealthProgress();

	if (bIsSignificanceEnabled)
	{
		USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(GetWorld());
		if (IsValid(SignificanceManager))
		{
			SignificanceManager->RegisterObject(
					this,
					SignificanceTagCharacter,
					[this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint) -> float
					{
						return SingnificanceFunction(ObjectInfo, ViewPoint);
					},
					USignificanceManager::EPostSignificanceType::Sequential,
					[this](USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
					{
						PostSignificanceFunction(ObjectInfo, OldSignificance, Significance, bFinal);
					}
			);
		}
	}
}

void AGCBaseCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (OnInteractableObjectFound.IsBound())
	{
		OnInteractableObjectFound.Unbind();

	}

	Super::EndPlay(Reason);
}

void AGCBaseCharacter::OnLevelDeserialized_Implementation()
{

}

void AGCBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGCBaseCharacter, bIsMantling);

}

void AGCBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AAIController* AIController = Cast<AAIController>(NewController);
	if (IsValid(AIController))
	{
		FGenericTeamId TeamId((uint8)Team);
		AIController->SetGenericTeamId(TeamId);
	}

}

void AGCBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TryChangeSprintState(DeltaTime);
	UpdateIkSetting(DeltaTime);

	TraceLineOfSight();
}

void AGCBaseCharacter::ChangeCrouchState()
{
	if (!GetCharacterMovement()->IsCrouching() && !GCBaseCharacterMovementComponent->IsProning() && (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()))
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}
}

void AGCBaseCharacter::ChangeProneState()
{
	if (GetCharacterMovement()->IsCrouching() && !GCBaseCharacterMovementComponent->IsProning())
	{
		Prone();
	}
	else if(!GetCharacterMovement()->IsCrouching() && GCBaseCharacterMovementComponent->IsProning())
	{
		UnProne(false);
	}
}

void AGCBaseCharacter::Prone(bool bClientSimulation /*= false*/)
{
	if (GCBaseCharacterMovementComponent)
	{
		if (CanProne())
		{
			GCBaseCharacterMovementComponent->bWantsToProne = true; //---

		}
	}

}

void AGCBaseCharacter::UnProne(bool bIsFullHeight, bool bClientSimulation)
{
	if (GCBaseCharacterMovementComponent)
	{
		GCBaseCharacterMovementComponent->bWantsToProne = false;
		GCBaseCharacterMovementComponent->bIsFullHeight = bIsFullHeight;
	}
}

bool AGCBaseCharacter::CanProne() const
{
	return !bIsProned && GCBaseCharacterMovementComponent && GetRootComponent() && !GetRootComponent()->IsSimulatingPhysics();
}

void AGCBaseCharacter::OnStartProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	RecalculateBaseEyeHeight();

	const ACharacter* DefaultChar = GetDefault<ACharacter>(GetClass());
	if (GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = GetMesh()->GetRelativeLocation().Z + HalfHeightAdjust + GCBaseCharacterMovementComponent->CrouchedHalfHeight - 34.0f;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = DefaultChar->GetBaseTranslationOffset().Z + HalfHeightAdjust + GCBaseCharacterMovementComponent->CrouchedHalfHeight;
	}

	K2_OnStartProne(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AGCBaseCharacter::OnEndProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	RecalculateBaseEyeHeight();

	const ACharacter* DefaultChar = GetDefault<ACharacter>(GetClass());
	const float HeightDifference = GCBaseCharacterMovementComponent->bIsFullHeight ? 0.0f : GCBaseCharacterMovementComponent->CrouchedHalfHeight - GCBaseCharacterMovementComponent->PronedHalfHeight;

	if (GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = GetMesh()->GetRelativeLocation().Z - 26.0f;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = GetBaseTranslationOffset().Z + HeightDifference;
	}

	K2_OnEndProne(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool AGCBaseCharacter::CanJumpInternal_Implementation() const
{

	return Super::CanJumpInternal_Implementation() && !GCBaseCharacterMovementComponent->IsOutOfStamina()
												   && !bIsProned
												   && !bIsCrouched
												   && !GetBaseCharacterMovementComponent()->IsMantling();

}

void AGCBaseCharacter::OnJumped_Implementation()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	if (GCBaseCharacterMovementComponent->IsProning())
	{
		UnProne(true);
	}
}

void AGCBaseCharacter::Falling()
{
	Super::Falling();
	GetBaseCharacterMovementComponent()->bNotifyApex = true;

}

void AGCBaseCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	CurrentFallApex = GetActorLocation();

}

void AGCBaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	float FallHeight = (CurrentFallApex - GetActorLocation()).Z * 0.01f;
	if (IsValid(FallDamageCurve))
	{
		float DamageAmount = FallDamageCurve->GetFloatValue(FallHeight);
		TakeDamage(DamageAmount, FDamageEvent(), GetController(), Hit.Actor.Get());
	}

}

void AGCBaseCharacter::StartSprint()
{		
	float GetSpeed = GetBaseCharacterMovementComponent()->Velocity.Size();

	if(CanJumpInternal_Implementation() && GetSpeed > 0.0f)
	{
		bIsSprintRequested = true;
		
	}
	if (bIsCrouched)
	{
		UnCrouch();
		StopSprint();
	}
	if (bIsProned)
	{
		UnProne(false);
		 
	}
}

void AGCBaseCharacter::StopSprint()
{
	bIsSprintRequested = false;

}

void AGCBaseCharacter::Mantle(bool bForce /*= false*/)
{
	if (!(CanMantle() || bForce))
	{
		return;
	}

	FLedgeDescription LedgeDescription;

	if (LedgeDetertorComponent->DetectLedge(LedgeDescription) &&  !GetBaseCharacterMovementComponent()->IsMantling() 
			&& !(GetBaseCharacterMovementComponent()->MovementMode == MOVE_Falling) /* && CanJumpInternal_Implementation()*/)
	{	
		bIsMantling = true;

		FMantlingMovementParameters MantlingParameters;
		MantlingParameters.MantlingCurve = HighMantleSettings.MantlingCurve;
		MantlingParameters.InitialLocation = GetActorLocation();
		MantlingParameters.InitialRotation = GetActorRotation();
		MantlingParameters.TargetLocation = LedgeDescription.Location;
		MantlingParameters.TargetRotation = LedgeDescription.Rotation;

		float MantlingHeight = (MantlingParameters.TargetLocation - MantlingParameters.InitialLocation).Z;

		const FMantlingSetting& MantlingSettings = GetMantlingSetting(MantlingHeight);

		float MinRange;
		float MaxRange;
		MantlingSettings.MantlingCurve->GetTimeRange(MinRange, MaxRange);

		MantlingParameters.Duration = MaxRange - MinRange;

		MantlingParameters.MantlingCurve = MantlingSettings.MantlingCurve;

		FVector2D SourceRange(MantlingSettings.MinHeight, MantlingSettings.MaxHeight);
		FVector2D TargetRange(MantlingSettings.MinHeightStartTime, MantlingSettings.MaxHeightStartTime);
		MantlingParameters.StartTime = FMath::GetMappedRangeValueClamped(SourceRange, TargetRange, MantlingHeight);
		 
		 MantlingParameters.InitialAnimationLocation = MantlingParameters.TargetLocation - MantlingSettings.AnimationCorrectionZ * FVector::UpVector + MantlingSettings.AnimationCorrectionXY * LedgeDescription.LedgeNormal;

		 if (IsLocallyControlled() || GetLocalRole() == ROLE_Authority)
		 {
			 GetBaseCharacterMovementComponent()->StartMantle(MantlingParameters);
		 }
		
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(MantlingSettings.MantlingMontage, 1.0f, EMontagePlayReturnType::Duration, MantlingParameters.StartTime);

		OnMantle(MantlingSettings, MantlingParameters.StartTime);
	}

}

void AGCBaseCharacter::OnRep_IsMantlong(bool bWasMantling)
{
	if (GetLocalRole() == ROLE_SimulatedProxy && !bWasMantling && bIsMantling)
	{
		Mantle(true);
	}
}

void AGCBaseCharacter::StartFire()
{	
	if (CharacterEquipmentComponent->IsSelectingWeapon())
	{
		return;
	}

	if (CharacterEquipmentComponent->IsEquipping())
	{
		return;
	}

	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{	
		CurrentRangeWeapon->StartFire();

	}
}


void AGCBaseCharacter::StopFire()
{
	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopFire();

	}
}

FRotator AGCBaseCharacter::GetAimOffset()
{
	FVector AimDirectionWorld = GetBaseAimRotation().Vector();
	FVector AimDirectionLocal = GetTransform().InverseTransformVectorNoScale(AimDirectionWorld);
	FRotator Result = AimDirectionLocal.ToOrientationRotator();
	
	return Result;
}

void AGCBaseCharacter::StartAiming()
{
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (!IsValid(CurrentRangeWeapon))
	{
		return;
	}

	bIsAiming = true;
	CurrentAimingMovementSpeed = CurrentRangeWeapon->GetAimMovementMaxSpeed();
	CurrentRangeWeapon->StartAim();
	OnStartAiming();
}

void AGCBaseCharacter::StopAiming()
{
	if (!bIsAiming)
	{
		return;
	}

	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopAim();

	}

	bIsAiming = false;
	CurrentAimingMovementSpeed = 0.0f;
	OnStopAiming();
}

bool AGCBaseCharacter::IsAiming() const
{
	return bIsAiming;
}

float AGCBaseCharacter::GetAimingMovementSpeed() const
{
	return CurrentAimingMovementSpeed;
}

void AGCBaseCharacter::OnStartAiming_Implementation()
{
	OnStartAimingInternal();

}

void AGCBaseCharacter::OnStopAiming_Implementation()
{
	OnStopAimingInternal();
}

void AGCBaseCharacter::Reload()
{
	if (IsValid(CharacterEquipmentComponent->GetCurrentRangeWeapon()))
	{
		CharacterEquipmentComponent->ReloadCurrentWeapon();
	}

}

void AGCBaseCharacter::NextItem()
{
	CharacterEquipmentComponent->EquipNextItem();
}

void AGCBaseCharacter::PreviousItem()
{
	CharacterEquipmentComponent->EquipPreviousItem();

}

void AGCBaseCharacter::EquipPrimaryItem()
{
	CharacterEquipmentComponent->EquipItemInSlot(EEquipmentSlots::PrimaryItemSlot);
}

const UCharacterEquipmentComponent* AGCBaseCharacter::GetCharacterEquipmentComponent() const
{
	return CharacterEquipmentComponent;
}

UCharacterEquipmentComponent* AGCBaseCharacter::GetCharacterEquipmentComponent_Muteble() const
{
	return CharacterEquipmentComponent;
}

const UCharacterAttributeComponent* AGCBaseCharacter::GetCharacterAttributeComponent() const
{
	return CharacterAttributeComponent;
}

UCharacterAttributeComponent* AGCBaseCharacter::GetCharacterAttributeComponent_Muteble() const
{
	return CharacterAttributeComponent;
}

void AGCBaseCharacter::RegisterInteractiveActor(AInteractiveActor* InteractiveActor)
{
	AvaibleInteractiveActors.AddUnique(InteractiveActor);

}

void AGCBaseCharacter::UnregisterInteractiveActor(AInteractiveActor* InteractiveActor)
{
	AvaibleInteractiveActors.RemoveSingleSwap(InteractiveActor);

}

void AGCBaseCharacter::ClimbLadderUp(float Value)
{
	if (GetBaseCharacterMovementComponent()->IsOnLadder() && !FMath::IsNearlyZero(Value))
	{
		FVector LadderUpVector =  GetBaseCharacterMovementComponent()->GetCurrentLadder()->GetActorUpVector();
		AddMovementInput(LadderUpVector, Value);
	}
}

void AGCBaseCharacter::InteractWithLadder()
{
	if (GetBaseCharacterMovementComponent()->IsOnLadder())
	{
		GetBaseCharacterMovementComponent()->DetachFromLadder(EDetachFromLadderMethod::JumpOff);

	}
	else
	{
		const ALadder* AvailableLadder = GetAvailableLadder();
		if (IsValid(AvailableLadder))
		{
			if (AvailableLadder->GetIsOnTop())
			{
				PlayAnimMontage(AvailableLadder->GetAttachFromTopAnimMontage());
			}
			GetBaseCharacterMovementComponent()->AttachToLadder(AvailableLadder);
		}

	}

}

const class ALadder* AGCBaseCharacter::GetAvailableLadder() const
{
	const ALadder* Result = nullptr;
	for (AInteractiveActor* InteractiveActor : AvaibleInteractiveActors)
	{
		if (InteractiveActor->IsA<ALadder>())
		{
			Result = StaticCast<const ALadder*>(InteractiveActor);
			break;

		}

	}

	return Result;
}

void AGCBaseCharacter::ClimbZipline(float Value)
{
	if (GetBaseCharacterMovementComponent()->IsOnZipline() && !FMath::IsNearlyZero(Value))
	{
		FVector ZiplineVector = GetBaseCharacterMovementComponent()->GetCurrentZipline()->GetActorForwardVector();


		//FRotator ZiplineVector = GetBaseCharacterMovementComponent()->GetCurrentZipline()->GetActorForwardVector().ToOrientationRotator() + 90.0f;
		//FVector NovZiplineVect = FVector(ZiplineVector.Roll,ZiplineVector.Pitch, ZiplineVector.Yaw);
	
		AddMovementInput(ZiplineVector, Value);
	}
}

void AGCBaseCharacter::InteractWithZipline()
{
	if (GetBaseCharacterMovementComponent()->IsOnZipline())
	{
		GetBaseCharacterMovementComponent()->DetachFromZipline(EDetachFromZiplineMethod::JumpOff);
	}
	else
	{
		const AZipline* AvailableZipline = GetAvailableZipline();
		if (IsValid(AvailableZipline))
		{
			GetBaseCharacterMovementComponent()->AttachToZipline(AvailableZipline);
		}
	}
}

const class AZipline* AGCBaseCharacter::GetAvailableZipline() const
{
	const AZipline* Result = nullptr;
	for (AInteractiveActor* InteractiveActor : AvaibleInteractiveActors)
	{
		if (InteractiveActor->IsA<AZipline>())
		{
			Result = StaticCast<const AZipline*>(InteractiveActor);
			break;
		}
	}
	return Result;
}

void AGCBaseCharacter::Stamina(bool bStamina)
{
	GetBaseCharacterMovementComponent()->SetIsOutOfStamina(bStamina);
}

bool AGCBaseCharacter::IsSwimmingUnderWater() const
{
	if (GetBaseCharacterMovementComponent()->IsSwimming())
	{	
		FVector HeadPosition = GetMesh()->GetSocketLocation(SocketHead);

		APhysicsVolume* Volume = GetBaseCharacterMovementComponent()->GetPhysicsVolume();
		float VolumeTopPlane = Volume->GetActorLocation().Z + Volume->GetBounds().BoxExtent.Z * Volume->GetActorScale3D().Z;
		if (VolumeTopPlane < HeadPosition.Z)
		{
			return true;
		}

	}
	return false;
}

void AGCBaseCharacter::PrimaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = CharacterEquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackType::PrimeryAttack);
	}

}

void AGCBaseCharacter::SecondaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = CharacterEquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackType::SecondaryAttack);
	}
}

void AGCBaseCharacter::Interact()
{
	if (LineOfSightObject.GetInterface())
	{
		LineOfSightObject->Interact(this);
	}
}

bool AGCBaseCharacter::PickupItem(TWeakObjectPtr<UInventoryItem> ItemToPickup)
{
	bool Result = false;
	if (CharacterInventoryComponent->HasFreeSlot())
	{
		CharacterInventoryComponent->AddItem(ItemToPickup, 1);
		Result = true;
	}
	return Result;

}

void AGCBaseCharacter::UseInventory(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (!CharacterInventoryComponent->IsViewVisible())
	{
		CharacterInventoryComponent->OpenViewInventory(PlayerController);
		CharacterEquipmentComponent->OpenViewEquipment(PlayerController);
		PlayerController->SetInputMode(FInputModeGameAndUI{});
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		CharacterInventoryComponent->CloseViewInventory();
		CharacterEquipmentComponent->CloseViewEquipment();
		PlayerController->SetInputMode(FInputModeGameOnly{});
		PlayerController->bShowMouseCursor = false;
	}

}

void AGCBaseCharacter::InitializeHealthProgress()
{
	UGCAttributeProgressBar* Widget = Cast<UGCAttributeProgressBar>(HealthBarProgressComponent->GetUserWidgetObject());
	if (!IsValid(Widget))
	{
		HealthBarProgressComponent->SetVisibility(false);
		return;
	}

	if (IsPlayerControlled() && IsLocallyControlled())
	{
		HealthBarProgressComponent->SetVisibility(false);
	}

	CharacterAttributeComponent->OnHealthChangedEvent.AddUObject(Widget, &UGCAttributeProgressBar::SetProgressPercantage);
	CharacterAttributeComponent->OnDeathEvent.AddLambda([=]() {	HealthBarProgressComponent->SetVisibility(false); });

	Widget->SetProgressPercantage(CharacterAttributeComponent->GetHealthPercet());


}

FGenericTeamId AGCBaseCharacter::GetGenericTeamId() const
{
	return FGenericTeamId((uint8)Team);
}

void AGCBaseCharacter::ConfirmWeaponSelection()
{
	if (CharacterEquipmentComponent->IsSelectingWeapon())
	{
		CharacterEquipmentComponent->ConfirmWeaponSelection();
	}
}

const FMantlingSetting& AGCBaseCharacter::GetMantlingSetting(float LedgeHeight) const
{
	
	return LedgeHeight > LowMantleMaxHeight ? HighMantleSettings : LowMantleSettings;
}


void AGCBaseCharacter::OnSprintStart_Implementation()
{

}

void AGCBaseCharacter::OnSprintEnd_Implementation()
{
}

bool AGCBaseCharacter::CanSprint()
{
	
	 return !GetBaseCharacterMovementComponent()->IsOutOfStamina() && FMath::IsNearlyEqual(GetVelocity().Rotation().Yaw, GetActorRotation().Yaw, 30.0f);
	
}

bool AGCBaseCharacter::CanMantle() const
{
	return !GetBaseCharacterMovementComponent()->IsOnLadder() && !GetBaseCharacterMovementComponent()->IsOnZipline();
}

void AGCBaseCharacter::OnMantle(const FMantlingSetting& MantlingSetting, float MantlingAnimationStartTime)
{
	
}

void AGCBaseCharacter::OnDeath()
{
	GetCharacterMovement()->DisableMovement();
	
	float Duration = PlayAnimMontage(OnDeathAnimMontage);
	if (Duration == 0.0f)
	{
		EnableRagdoll();
	}

}

void AGCBaseCharacter::OnStartAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(true);
	}
}

void AGCBaseCharacter::OnStopAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(false);
	}
}

void AGCBaseCharacter::TraceLineOfSight()
{
	if (!IsPlayerControlled())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	APlayerController* PlayerController = GetController<APlayerController>();
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector ViewDirection = ViewRotation.Vector();

	FVector TraceEnd = ViewLocation + ViewDirection * LineOfSightDistance;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility);
	
	if (LineOfSightObject.GetObject() != HitResult.Actor)
	{
		LineOfSightObject = HitResult.Actor.Get();
			
		FName ActionName;
		if (LineOfSightObject.GetInterface())
		{
			ActionName = LineOfSightObject->GetActionEventName();
			
		}
		else
		{
			ActionName = NAME_None;
		}

		OnInteractableObjectFound.ExecuteIfBound(ActionName);

	}
}

float AGCBaseCharacter::SingnificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint)
{
	if (ObjectInfo->GetTag() == SignificanceTagCharacter)
	{
		AGCBaseCharacter* Character = StaticCast<AGCBaseCharacter*>(ObjectInfo->GetObject());
		if (!IsValid(Character))
		{
			return SignificanceValueVeryHith;

		}

		if (Character->IsPlayerControlled() && Character->IsLocallyControlled())
		{
			return SignificanceValueVeryHith;

		}
		
		float DistSquared = FVector::DistSquared(Character->GetActorLocation(), ViewPoint.GetLocation());
		
		if (DistSquared <= FMath::Square(VeryHighSignificanceDistance))
		{
			return SignificanceValueVeryHith;
		}
		else if(DistSquared <= FMath::Square(HighSignificanceDistance))
		{
			return SignificanceValueHith;
		}
		else if(DistSquared <= FMath::Square(MediumSignificanceDistance))
		{
			return SignificanceValueMedium;
		}
		else if (DistSquared <= FMath::Square(LowSignificanceDistance))
		{
			return SignificanceValueLow;
		}
		else
		{
			return SignificanceValueVeryLow;
		}
	}

	return SignificanceValueVeryHith;
}

void AGCBaseCharacter::PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
	if (OldSignificance == Significance)
	{
		return;
	}

	if (ObjectInfo->GetTag() != SignificanceTagCharacter)
	{
		return;
	}

	AGCBaseCharacter* Character = StaticCast<AGCBaseCharacter*>(ObjectInfo->GetObject());
	if (!IsValid(Character))
	{
		return;
	}
	
	UCharacterMovementComponent* MovementComponent = Character->GetBaseCharacterMovementComponent();
	AAIController* AIController = Character->GetController<AAIController>();
	UWidgetComponent* Widget =  Character->HealthBarProgressComponent;

	if (Significance == SignificanceValueVeryHith)
	{
		MovementComponent->SetComponentTickInterval(0.0f);
		Widget->SetVisibility(true);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.0f);

		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.0f);

		}

	}
	else if (Significance == SignificanceValueHith)
	{
		MovementComponent->SetComponentTickInterval(0.0f);
		Widget->SetVisibility(true);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.05f);

		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.0f);

		}

	}
	else if (Significance == SignificanceValueMedium)
	{
		MovementComponent->SetComponentTickInterval(0.1f);
		Widget->SetVisibility(false);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.1f);

		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.1f);

		}
	}
	else if (Significance == SignificanceValueLow)
	{
		MovementComponent->SetComponentTickInterval(1.0f);
		Widget->SetVisibility(false);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(1.0f);

		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(1.0f);

		}
	}
	else if (Significance == SignificanceValueVeryLow)
	{
		MovementComponent->SetComponentTickInterval(5.0f);
		Widget->SetVisibility(false);
		Character->GetMesh()->SetComponentTickEnabled(false);

		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(10.0f);

		}
	}

}

void AGCBaseCharacter::UpdateIkSetting(float DeltaSeconds)
{

	IKLeftFootSocketOffset = FMath::FInterpTo(IKLeftFootSocketOffset, GetIKOffsetForSocket(LeftFootSocketName), DeltaSeconds, IKInterpSpeed);
	IKRightFootSocketOffset = FMath::FInterpTo(IKRightFootSocketOffset, GetIKOffsetForSocket(RightFootSocketName), DeltaSeconds, IKInterpSpeed);
	IKPelvisOffset = FMath::FInterpTo(IKPelvisOffset, CalculateIKPelvisOffset(), DeltaSeconds, IKInterpSpeed);

}

//
float AGCBaseCharacter::GetIKOffsetForSocket(const FName& SocketName) const
{
	float Result = 0.0f;

	float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector SocketLocation = GetMesh()->GetSocketLocation(SocketName);
	FVector TraceStart(SocketLocation.X, SocketLocation.Y, GetActorLocation().Z);
	FVector TraceEnd = TraceStart - (CapsuleHalfHeight + IKTraceDistance) * FVector::UpVector;

	FHitResult HitResult;

	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd, TraceType, true, TArray<AActor*>(), EDrawDebugTrace::ForOneFrame, HitResult, true))
	{	
		float CharacterBottom = TraceStart.Z - CapsuleHalfHeight;
		Result = CharacterBottom - HitResult.Location.Z;	
	}

	return Result;
}

float AGCBaseCharacter::CalculateIKPelvisOffset()
{
	return -FMath::Abs(IKRightFootSocketOffset - IKLeftFootSocketOffset);
}

void AGCBaseCharacter::TryChangeSprintState(float DeltaTime)
{
	if (bIsSprintRequested && !GCBaseCharacterMovementComponent->IsSprinting() && CanSprint())
	{
		GCBaseCharacterMovementComponent->StartSprint();
		OnSprintStart(); 

	}
	if (!(bIsSprintRequested && CanSprint()) && GCBaseCharacterMovementComponent->IsSprinting())
	{
		GCBaseCharacterMovementComponent->StopSprint();
		OnSprintEnd();
	}
	
	
}

void AGCBaseCharacter::EnableRagdoll()
{
	GetMesh()->SetCollisionProfileName(CollisionProfileRagdoll);
	GetMesh()->SetSimulatePhysics(true);
}



