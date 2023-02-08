// Fill out your copyright notice in the Description page of Project Settings.


#include "Zipline.h"
#include "../../../GameCodeTypes.h"
#include <Components/SceneComponent.h>
#include <Components/StaticMeshComponent.h>
#include <Components/BoxComponent.h>


AZipline::AZipline()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ZiplineRoot"));

	TopRailMechComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopRail"));
	TopRailMechComponent->SetupAttachment(RootComponent);

	BottomRailMechComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottomRail"));
	BottomRailMechComponent->SetupAttachment(RootComponent);

	CableMechComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cable"));
	CableMechComponent->SetupAttachment(RootComponent);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionValue"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetCollisionProfileName(CollisionProfilePawnInteractionVolume);
	InteractionVolume->SetGenerateOverlapEvents(true);



}

void AZipline::OnConstruction(const FTransform& Transform)
{	
	TopRailMechComponent->SetRelativeLocation(FVector(0.0f, -ZiplineWigth * 0.5f, ZiplineHeight * 0.5f));
	BottomRailMechComponent->SetRelativeLocation(FVector(0.0f, ZiplineWigth * 0.5f, ZiplineHeight * 0.5f));
	CableMechComponent->SetRelativeLocation(FVector(0.0f, 0.0f, ZiplineHeight));

	UStaticMesh* TopRailMesh = TopRailMechComponent->GetStaticMesh();
	if (IsValid(TopRailMesh))
	{
		float MeshHeight = TopRailMesh->GetBoundingBox().GetSize().Z;
		if (!FMath::IsNearlyZero(MeshHeight))
		{
			TopRailMechComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, ZiplineHeight /MeshHeight));
		}
	}

	UStaticMesh* BottomRailMesh = BottomRailMechComponent->GetStaticMesh();
	if (IsValid(BottomRailMesh))
	{
		float MeshHeight = BottomRailMesh->GetBoundingBox().GetSize().Z;
		if (!FMath::IsNearlyZero(MeshHeight))
		{
			BottomRailMechComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, ZiplineHeight / MeshHeight));
		}
	}

	UStaticMesh* CableMesh = CableMechComponent->GetStaticMesh();
	if (IsValid(CableMesh))
	{
		float MeshWigth = CableMesh->GetBoundingBox().GetSize().Y;
		if (!FMath::IsNearlyZero(MeshWigth))
		{
			CableMechComponent->SetRelativeScale3D(FVector(1.0f, ZiplineWigth / MeshWigth, 1.0f));
		}
	}

	float BoxDepthExtent = GetZiplineInteractionBox()->GetScaledBoxExtent().X;
	GetZiplineInteractionBox()->SetBoxExtent(FVector(BoxDepthExtent, ZiplineWigth * 0.5f, ZiplineInteractionBoxZ));
	GetZiplineInteractionBox()->SetRelativeLocation(FVector(0.0f, 0.0f, ZiplineHeight - ZiplineInteractionBoxZ));
	

}

void AZipline::BeginPlay()
{
	Super::BeginPlay();


}

float AZipline::GetZiplineHeight() const
{
	return ZiplineHeight;
}

UBoxComponent* AZipline::GetZiplineInteractionBox() const
{
	return StaticCast<UBoxComponent*>(InteractionVolume);
}

void AZipline::OnInteractionVolueOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnInteractionVolueOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

}

void AZipline::OnInteractionVolueOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnInteractionVolueOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

}
