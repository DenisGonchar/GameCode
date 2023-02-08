// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include <Inventory/Items/InventoryItem.h>

/**
 * 
 */
namespace GCDataTableUtils
{
	FWeaponTableRow* FindWeaponData( const FName WeaponID );
	FItemTableRow* FindInventoryItemData(const FName ItemID);
};
