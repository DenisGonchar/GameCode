// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include "GameCodeTypes.h"
#include "Components/Weapon/MeleeHitRegistrator.h"
#include "MeleeWeaponItem.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FMeleeAttackDecription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee attack")
	TSubclassOf<class UDamageType> DamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee attack", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float DamageAmount = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee attack")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee attack")
	bool bIsMoveAttack = false;
};


UCLASS(Blueprintable)
class GAMECODE_API AMeleeWeaponItem : public AEquipableItem
{
	GENERATED_BODY()
	
public:
	AMeleeWeaponItem();

	void StartAttack(EMeleeAttackType AttackType);

	void SetIsHitRegistrationEnabled( bool bIsregistrationEnanled);



protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee attack")
	TMap<EMeleeAttackType, FMeleeAttackDecription> Attacks;

	virtual void BeginPlay() override;

private:
	
	UFUNCTION()
	void ProcessHit(const FHitResult& HitResult, const FVector& HitDirection);

	TArray<UMeleeHitRegistrator*> HitRegistrators;
	TSet<AActor* > HitActors;

	FMeleeAttackDecription* CurrentAttack;
	
	void OnAttackTimerElapsed();

	FTimerHandle AttackTimer;

};
