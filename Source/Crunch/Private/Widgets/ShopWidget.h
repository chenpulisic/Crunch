// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UTileView;
class UPA_ShopItem;
class UInventoryComponent;
class UShopItemWidget;
/**
 * 
 */
UCLASS()
class UShopWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
private:
	UPROPERTY(meta=(BindWidget))
	UTileView* ShopItemList;

	UPROPERTY(meta = (BindWidget))
	class UItemTreeWidget* CombinationTree;

	void LoadShopItems();
	void ShopItemLoadFinished();
	void ShopItemWidgetGenerated(UUserWidget& NewWidget);

	UPROPERTY()
	TMap<const UPA_ShopItem*, const UShopItemWidget*> ItemsMap;

	UPROPERTY()
	UInventoryComponent* OwnerInventoryComponent;

	void ShowItemCombinatio(const UShopItemWidget* ItemWidget);
};
