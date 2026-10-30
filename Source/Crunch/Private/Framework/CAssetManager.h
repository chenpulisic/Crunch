// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/PA_ShopItem.h"
#include "Engine/AssetManager.h"
#include "CAssetManager.generated.h"

class UPA_CharacterDefinition;
/**
 * 
 */
UCLASS()
class UCAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	static UCAssetManager& Get();

	void LoadCharacterDefinations(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedCharacterDefinitions(TArray<UPA_CharacterDefinition*>& LoadedCharacterDefinitions) const;

	void LoadShopItems(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedShopItems(TArray<const UPA_ShopItem*>& OutItems) const;
	const FItemCollection* GetCombinationForItem(const UPA_ShopItem* Item) const;
	const FItemCollection* GetIngredientForItem(const UPA_ShopItem* Item) const;
private:
	void ShopItemLoadFinished(const FStreamableDelegate Callback);
	void BuildItemMaps();
	void AddToCombinationMap(const UPA_ShopItem* Ingredient, const UPA_ShopItem* CombinationItem);

	UPROPERTY()
	TMap<const UPA_ShopItem*, FItemCollection> CombinationMap;
	UPROPERTY()
	TMap<const UPA_ShopItem*, FItemCollection> IngredientMap;
};
