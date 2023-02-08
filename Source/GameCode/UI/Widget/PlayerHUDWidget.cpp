// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/PlayerHUDWidget.h"
#include "Pawns/Character/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"
#include "Blueprint/WidgetTree.h"
#include "ReticleWidget.h"
#include "AmmoWidget.h"
#include "CharacterAttributesWidget.h"
#include "GrenadeWidget.h"
#include "HighlightInteractable.h"

UReticleWidget* UPlayerHUDWidget::GetReticalWidget()
{
	return WidgetTree->FindWidget<UReticleWidget>(ReticleWidgetName);
	
}

class UAmmoWidget* UPlayerHUDWidget::GetAmmoWidget()
{
	return WidgetTree->FindWidget<UAmmoWidget>(AmmoWidgetName);
}

class UGrenadeWidget* UPlayerHUDWidget::GetGrenadeWidget()
{
	return WidgetTree->FindWidget<UGrenadeWidget>(GrenadeWidgetName);
}

class UCharacterAttributesWidget* UPlayerHUDWidget::GetCharacterAttributesWidget()
{
	return WidgetTree->FindWidget<UCharacterAttributesWidget>(AttributesWidgetName);
}

void UPlayerHUDWidget::SetHighlightInteractebleVisibility(bool bIsVisible)
{
	if (!IsValid(InteractableKey))
	{
		return;
	}

	if (bIsVisible)
	{
		InteractableKey->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		InteractableKey->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPlayerHUDWidget::SetHightInteractableActionText(FName KeyName)
{
	if (IsValid(InteractableKey))
	{
		InteractableKey->SetActionText(KeyName);
	}
}

float UPlayerHUDWidget::GetHealthPercent() const
{
	float Result = 1.0f;
	
	APawn* Pawn = GetOwningPlayerPawn();
	AGCBaseCharacter* Character = Cast<AGCBaseCharacter>(Pawn);
	if (IsValid(Character))
	{
		const UCharacterAttributeComponent* CharacterAttributes = Character->GetCharacterAttributeComponent();

		Result = CharacterAttributes->GetHealthPercet();

	}

	return Result;
}
