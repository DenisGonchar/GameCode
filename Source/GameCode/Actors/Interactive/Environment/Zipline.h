// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../InteractiveActor.h"
#include "Zipline.generated.h"

class UStaticMeshComponent;
class UAnimMontage;
class UBoxComponent;

UCLASS(Blueprintable)
class GAMECODE_API AZipline : public AInteractiveActor
{
	GENERATED_BODY()
	
public:
	AZipline();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	float GetZiplineHeight() const;

protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder parameters", meta = (ClampMin = 30.0f, UIMin = 30.0f))
	float ZiplineHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder parameters", meta = (ClampMin = 30.0f, UIMin = 30.0f))
	float ZiplineWigth = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder parameters")
	float ZiplineInteractionBoxZ = 20.0f;


	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline parameters")
	FVector PositionTopRailMech = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline parameters")
	FVector PositionBottomRailMesh = FVector::ZeroVector;*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* TopRailMechComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* BottomRailMechComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* CableMechComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline Parameters")
	UAnimMontage* AttachFromTopAnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline Parameters")
	FVector AttachFromTopAnimMontageIntialOffset = FVector::ZeroVector;

	UBoxComponent* GetZiplineInteractionBox() const;

	virtual void OnInteractionVolueOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void OnInteractionVolueOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

private:


};
