// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include <Subsystems/SaveSubsystem/SaveSubsystemInterface.h>
#include "RangeWeaponItem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnReloadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32);

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Single, 
	FullAuto

};

UENUM(BlueprintType)
enum class EReloadType : uint8
{
	FullClip,
	ByBullet


};


class UAnimMontage;

UCLASS(Blueprintable)
class GAMECODE_API ARangeWeaponItem : public AEquipableItem, public ISaveSubsystemInterface
{
	GENERATED_BODY()
	
public:
	ARangeWeaponItem();

	void StartFire();
	void StopFire();
	
	bool IsFiring() const;

	void StartAim();
	void StopAim();

	float GetAimFOV() const;
	float GetAimMovementMaxSpeed() const;
	float GetAimTurnModifier() const;
	float GetAimLookUpModifier() const;

	FTransform GetForeGripTransform(); 

	//Ammo
	int32 GetAmmo() const;
	int32 GetMaxAmmo() const;
	void SetAmmo(int32 NewAmmo);
	bool CanShoot() const;

	FOnAmmoChanged OnAmmoChanged;

	EAmmunitionType GetAmmoType() const;

	//Reload
	void StartReload();
	void EndReload(bool bIsSuccess);

	bool IsReloadnig() const;

	FOnReloadComplete OnReloadComplete;

	virtual EReticleType GetReticleType() const;

	virtual void OnLevelDeserialized_Implementation() override;

protected:
	
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWeaponBarellComponent* WeaponBarell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	UAnimMontage* WeaponFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	UAnimMontage* WeaponReloadMontage;

	//FullClip reload type adds ammo only when the whole reload animation is successfully played
	//ByBullet reload type requires section "ReloadEnd" in character reload animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	EReloadType ReloadType = EReloadType::FullClip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Character")
	UAnimMontage* CharacterFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Character")
	UAnimMontage* CharacterReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters")
	EWeaponFireMode WeaponFireMode = EWeaponFireMode::Single;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo", meta = (ClampMin = 1.0f, UIMin = 1.0f))
	EAmmunitionType AmmoType = EAmmunitionType::None;

	//Rate of fire in round per minute
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters", meta = (ClampMin = 1.0f, UIMin = 1.0f))
	float RateOfFire = 600.0f;

	//Bullet spread half angle in degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2.0f, UIMax = 2.0f))
	float SpreadAngle = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aimimg", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2.0f, UIMax = 2.0f))
	float AimSpreadAngle = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aimimg", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AimMovementMaxSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aimimg", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 120.0f, UIMax = 120.0f))
	float AimFOV = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aimimg", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 1.0f, UIMax = 1.0f))
	float AimTurnModifier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aimimg", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 1.0f, UIMax = 1.0f))
	float AimLookUpModifier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo", meta = (ClampMin = 1.0f, UIMin = 1.0f))
	int32 MaxAmmo = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo")
	bool bAutoReload = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reticle")
	EReticleType AimReticleType = EReticleType::Default;

private:
	UPROPERTY(SaveGame)
	int32 Ammo = 0.0f;

	bool bIsAiming;
	bool bIsReloading = false;
	bool bIsFiring = false;

	float GetCurrentBulletSpreadAngle() const;
	void MakeShot();

	void OnShotTimerElapsed();

	float GetShotTimerInterval() const;
	float PlayAnimMontage(UAnimMontage* AnimMontage);
	void StopAnimMontage(UAnimMontage* AnimMontage, float BlendOutTime = 0.0f);

	FTimerHandle ShotTimer;
	FTimerHandle ReloadTimer;
};
