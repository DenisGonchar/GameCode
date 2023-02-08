// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Weapon/WeaponBarellComponent.h"
#include "GameCodeTypes.h"
#include "DrawDebugHelpers.h"
#include <Subsystems/DebugSubsystem.h>
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/DecalComponent.h"
#include "Actors/Projectiles/GCProjectile.h"
#include "Net/UnrealNetwork.h"


UWeaponBarellComponent::UWeaponBarellComponent()
{
	SetIsReplicatedByDefault(true);

}

void UWeaponBarellComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() < ROLE_Authority)
	{
		return;
	}

	SaveCurveDamage = DamageAmount;

	if (!IsValid(ProjectileClass))
	{
		return;
	}

	ProjectilePool.Reserve(ProjectilePoolSize);
	for (int32 i = 0; i < ProjectilePoolSize; ++i)
	{
		AGCProjectile* Projectile = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, ProjectilePoolLocation, FRotator::ZeroRotator);
		Projectile->SetOwner(GetOwnerPawn());
		Projectile->SetProjectileActive(false);
		ProjectilePool.Add(Projectile);

	}


	
}

void UWeaponBarellComponent::Shot(FVector ShotStart, FVector ShotDirection, float SpreadAngle)
{
	TArray<FShotInfo> ShotInfo;
	
	for (int i = 0; i < BulletPerShot; i++)
	{
		ShotDirection += GetBulletSpreadOffset(FMath::RandRange(0.0f, SpreadAngle), ShotDirection.ToOrientationRotator());
		ShotDirection = ShotDirection.GetSafeNormal();
		
		ShotInfo.Emplace(ShotStart, ShotDirection);
	
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_Shot(ShotInfo);
	}
	
	ShotInternal(ShotInfo);

}

void UWeaponBarellComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;

	RepParams.Condition = COND_SimulatedOnly;
	RepParams.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS(UWeaponBarellComponent, LastShotsInfo, RepParams);
	DOREPLIFETIME(UWeaponBarellComponent, ProjectilePool);
	DOREPLIFETIME(UWeaponBarellComponent, CurrentProjectileIndex);
	DOREPLIFETIME(UWeaponBarellComponent, SaveCurveDamage);

}

bool UWeaponBarellComponent::HitScan(FVector ShotStart, OUT FVector& ShotEnd, FVector ShotDirection)
{
	FHitResult ShotResult;
	bool bHasHit = GetWorld()->LineTraceSingleByChannel(ShotResult, ShotStart, ShotEnd, ECC_Bullet);
	if (bHasHit)
	{
		ShotEnd = ShotResult.ImpactPoint;

		float ForwardDistance = (MuzzleLocation - ShotEnd).Size();
		if (IsValid(FalloffDiagrama))
		{
			float CurveValue = FalloffDiagrama->GetFloatValue(ForwardDistance);

			if (CurveValue > 0.0f && SaveCurveDamage > 0.0f)
			{

				DamageAmount = SaveCurveDamage + (CurveValue * SaveCurveDamage);

			}

		}

		ProcessHit(ShotResult, ShotDirection);
	}


	return bHasHit;
}


void UWeaponBarellComponent::ProcessProjectileHit(AGCProjectile* Projectile, const FHitResult& HitResult, const FVector& Direction)
{
	Projectile->SetProjectileActive(false);
	Projectile->SetActorLocation(ProjectilePoolLocation);
	Projectile->SetActorRotation(FRotator::ZeroRotator);
	Projectile->OnProjectileHit.RemoveAll(this);

	ProcessHit(HitResult, Direction);
}

void UWeaponBarellComponent::ProcessHit(const FHitResult& HitResult, const FVector& Direction)
{
	AActor* HitActor = HitResult.GetActor();

	if (GetOwner()->HasAuthority() && IsValid(HitActor))
	{
		FPointDamageEvent DamageEvent;
		DamageEvent.HitInfo = HitResult;
		DamageEvent.ShotDirection = Direction;
		DamageEvent.DamageTypeClass = DamageTypeClass;

		HitActor->TakeDamage(DamageAmount, DamageEvent, GetController(), GetOwner());

		UE_LOG(LogTemp, Warning, TEXT("Damage = %f"), DamageAmount);

	}

	UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), DefaultDecalInfo.DecalMaterial, DefaultDecalInfo.DecalSize, HitResult.ImpactPoint, HitResult.ImpactNormal.ToOrientationRotator());
	if (IsValid(DecalComponent))
	{
		DecalComponent->SetFadeScreenSize(0.0001f);
		DecalComponent->SetFadeOut(DefaultDecalInfo.DecalLifeTime, DefaultDecalInfo.DecalFadeOutTime);
	}
}


void UWeaponBarellComponent::LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDirection)
{
	
	AGCProjectile* Projectile = ProjectilePool[CurrentProjectileIndex];
	
	Projectile->SetActorLocation(LaunchStart);
	Projectile->SetActorRotation(LaunchDirection.ToOrientationRotator());

	Projectile->SetProjectileActive(true);
	Projectile->OnProjectileHit.AddDynamic(this, &UWeaponBarellComponent::ProcessProjectileHit);
	Projectile->LaunchProjectile(LaunchDirection.GetSafeNormal());
	
	++CurrentProjectileIndex;
	if (CurrentProjectileIndex == ProjectilePool.Num())
	{
		CurrentProjectileIndex = 0;
	}

}

void UWeaponBarellComponent::ShotInternal(const TArray<FShotInfo>& ShotsInfo)
{	
	if(GetOwner()->HasAuthority())
	{
		LastShotsInfo = ShotsInfo;	
	}

	MuzzleLocation = GetComponentLocation();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation, GetComponentRotation());

	for (const FShotInfo& ShotInfo : ShotsInfo)
	{
		FVector ShotStart = ShotInfo.GetLocation();
		FVector ShotDirection = ShotInfo.GetDirection();
		FVector ShotEnd = ShotStart + FiringRange * ShotDirection;

#if ENABLE_DRAW_DEBUG
		UDebugSubsystem* DebugSubSystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
		bool bIsDebugEnabled = DebugSubSystem->IsCategoryEnabled(DebugCategoryRangeWeapon);
#else
		bool bIsDebugEnabled = false;
#endif
		switch (HitRegistration)
		{
			case EHitRegistrationType::HitScan:
			{
				bool bHasHit = HitScan(ShotStart, ShotEnd, ShotDirection);

				if (bIsDebugEnabled && bHasHit)
				{
					DrawDebugSphere(GetWorld(), ShotEnd, 10.0f, 24, FColor::Red, false, 5.0f);
				}

				break;
			}

			case EHitRegistrationType::Projectile:
			{
				LaunchProjectile(ShotStart, ShotDirection);

				break;
			}

			default:
				break;
		}



		UNiagaraComponent* TraceFXComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TraceFX, MuzzleLocation, GetComponentRotation());
		if (IsValid(TraceFXComponent))
		{
			TraceFXComponent->SetVectorParameter(FXParamTraceEnd, ShotEnd);
		}

		if (bIsDebugEnabled)
		{
			DrawDebugLine(GetWorld(), MuzzleLocation, ShotEnd, FColor::Red, false, 1.0f, 0, 5.0f);

		}
	}
}

void UWeaponBarellComponent::Server_Shot_Implementation(const TArray<FShotInfo>& ShotsInfo)
{
	ShotInternal(ShotsInfo);

}

void UWeaponBarellComponent::OnRep_LastShotsInfo()
{
	ShotInternal(LastShotsInfo);
}

APawn* UWeaponBarellComponent::GetOwnerPawn() const
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!IsValid(PawnOwner))
	{
		PawnOwner = PawnOwner = Cast<APawn>(GetOwner()->GetOwner());
	}
	return PawnOwner;
}

AController* UWeaponBarellComponent::GetController() const
{
	APawn* PawnOwner = GetOwnerPawn();
	return IsValid(PawnOwner) ? PawnOwner->GetController() : nullptr;
}

FVector UWeaponBarellComponent::GetBulletSpreadOffset(float Angle, FRotator ShotRotation) const
{
	float SpreadSize = FMath::Tan(Angle);
	float RotationAngle = FMath::RandRange(0.0f, 2 * PI);

	float SpreadY = FMath::Cos(RotationAngle);
	float SpreadZ = FMath::Sin(RotationAngle);

	FVector Result = (ShotRotation.RotateVector(FVector::UpVector) * SpreadZ + ShotRotation.RotateVector(FVector::RightVector) * SpreadY) * SpreadSize;

	return Result;
}

