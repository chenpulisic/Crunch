// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/PA_ShopItem.h"
#include "ItemToolTip.generated.h"

class UPA_ShopItem;
/**
 * 
 */
UCLASS()
class UItemToolTip : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetItem(const UPA_ShopItem* Item);
	void SetPrice(float newPrice);
private:
	UPROPERTY(meta = (BindWIdget))
	class UImage* IconImage;

	UPROPERTY(meta = (BindWIdget))
	class UTextBlock* ItemTitleText;

	UPROPERTY(meta = (BindWIdget))
	class UTextBlock* ItemDescriptionText;

	UPROPERTY(meta = (BindWIdget))
	class UTextBlock* ItemPriceText;
};
