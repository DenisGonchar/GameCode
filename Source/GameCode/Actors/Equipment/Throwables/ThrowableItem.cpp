// Fill out your copyright notice in the Description page of Project Settings.


#include "ThrowableItem.h"
#include "Pawns/Character/GCBaseCharacter.h"
#include "../../Projectiles/GCProjectile.h"

void AThrowableItem::Throw()
{
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	FVector PlayerViewPoint;
	FRotator PlayerViewRotation;

	if (CharacterOwner->IsPlayerControlled())
	{
		APlayerController* Controller = CharacterOwner->GetController<APlayerController>();
		Controller->GetPlayerViewPoint(PlayerViewPoint, PlayerViewRotation);

	}
	else
	{
		PlayerViewPoint = GetActorLocation();
		PlayerViewRotation = CharacterOwner->GetBaseAimRotation();
	}

	FTransform PlayerViewTransform(PlayerViewRotation, PlayerViewPoint);

	FVector ViewDirection = PlayerViewRotation.RotateVector(FVector::ForwardVector);
	FVector ViewUpVector = PlayerViewRotation.RotateVector(FVector::UpVector);

	FVector LaunchDirection = ViewDirection + FMath::Tan(FMath::DegreesToRadians(ThrowAngle)) * ViewUpVector;

	FVector ThrowableSocketLocation = CharacterOwner->GetMesh()->GetSocketLocation(SocketCharacterThrowable);
	FVector SocketInViewSpace = PlayerViewTransform.InverseTransformPosition(ThrowableSocketLocation);

	FVector SpawnLocation = PlayerViewPoint + ViewDirection * SocketInViewSpace.X;

	AGCProjectile* Projectile = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, SpawnLocation, FRotator::ZeroRotator);
	if (IsValid(Projectile))
	{
		Projectile->SetOwner(GetOwner());
		Projectile->LaunchProjectile(LaunchDirection.GetSafeNormal());
	}


}
 