// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InventoryWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/InventoryItemWidget.h"
#include "Widgets/InventoryContextMenuWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//UE_LOG(LogTemp, Warning, TEXT("[NativeConstruct] ContextMenuWidgetClass=%s, ItemWidgetClass=%s"),
		//*GetNameSafe(ContextMenuWidgetClass), *GetNameSafe(ItemWidgetClass));

	 
	if (APawn* OwnerPawn = GetOwningPlayerPawn())
	{
		InventoryComponent = OwnerPawn->GetComponentByClass<UInventoryComponent>();
		if (InventoryComponent)
		{
			InventoryComponent->OnItemAdded.AddUObject(this, &UInventoryWidget::ItemAdded);
			InventoryComponent->OnItemRemoved.AddUObject(this, &UInventoryWidget::ItemRemoved);
			InventoryComponent->OnItemStackCountChanged.AddUObject(this, &UInventoryWidget::ItemStackCountChanged);
			int Capacity = InventoryComponent->GetCapacity();

			ItemList->ClearChildren();

			for(int i = 0; i<Capacity; ++i)
			{
				UInventoryItemWidget* NewEmptyWidget = CreateWidget<UInventoryItemWidget>(GetOwningPlayer(), ItemWidgetClass);
				if(NewEmptyWidget)
				{
					NewEmptyWidget->SetSlotNumber(i);
					UWrapBoxSlot* NewItemSlot = ItemList->AddChildToWrapBox(NewEmptyWidget);
					NewItemSlot->SetPadding(FMargin(2.f));
					ItemWidgets.Add(NewEmptyWidget);

					NewEmptyWidget->OnInventoryItemDropped.AddUObject(this, &UInventoryWidget::HandleItemDragDrop);
					NewEmptyWidget->OnLeftButtonClicked.AddUObject(InventoryComponent, &UInventoryComponent::TryActivateItem);
					NewEmptyWidget->OnRightButtonClicked.AddUObject(this, &UInventoryWidget::ToggleContextMenu);
				}
			}
			SpawnContextMenu();
		}
	}
}

void UInventoryWidget::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);

	if (!ContextMenuWidget)
	{
		return;  // 安全返回，不访问空指针
	}

	TSharedPtr<SWidget> CachedSWidget = ContextMenuWidget->GetCachedWidget();
	if (!CachedSWidget.IsValid())
	{
		return;
	}

	if (!NewWidgetPath.ContainsWidget(CachedSWidget.Get()))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[FocusChanging] Focus left ContextMenu, clearing"));
		ClearContextMenu();
	}
	/*if (!NewWidgetPath.ContainsWidget(ContextMenuWidget->GetCachedWidget().Get()))
	{
		ClearContextMenu();
	}*/
}

void UInventoryWidget::SpawnContextMenu()
{
	if (!ContextMenuWidgetClass)
	{
		//UE_LOG(LogTemp, Error, TEXT("[SpawnContextMenu] FAILED: ContextMenuWidgetClass is NULL! Set it in the InventoryWidget blueprint!"));
		return;
	}
	ContextMenuWidget = CreateWidget<UInventoryContextMenuWidget>(this, ContextMenuWidgetClass);
	if (ContextMenuWidget)
	{
		ContextMenuWidget->GetSellButtonClickedEvent().AddDynamic(this, &UInventoryWidget::SellFocusedItem);
		ContextMenuWidget->GetUseButtonClickedEvent().AddDynamic(this, &UInventoryWidget::UseFocusedItem);
		ContextMenuWidget->AddToViewport(1);
		SetContextMenuVisible(false);

		//UE_LOG(LogTemp, Warning, TEXT("[SpawnContextMenu] SUCCESS: Created and added to viewport"));
	}
	/*else
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnContextMenu] FAILED: CreateWidget returned NULL! Check ContextMenuWidgetClass blueprint validity"));
	}*/
}

void UInventoryWidget::SellFocusedItem()
{
	//UE_LOG(LogTemp, Warning, TEXT("Selling Item"));
	InventoryComponent->SellItem(CurrentFocusedItemHandle);
	SetContextMenuVisible(false);
}

void UInventoryWidget::UseFocusedItem()
{
	//UE_LOG(LogTemp, Warning, TEXT("Using Item"));
	InventoryComponent->TryActivateItem(CurrentFocusedItemHandle);
	SetContextMenuVisible(false);
}

void UInventoryWidget::SetContextMenuVisible(bool bContextMenuVisible)
{
	if (ContextMenuWidget)
	{
		ContextMenuWidget->SetVisibility(bContextMenuVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		UE_LOG(LogTemp, Warning, TEXT("[SetContextMenuVisible] -> %s"), bContextMenuVisible ? TEXT("Visible") : TEXT("Hidden"));
	}
}

void UInventoryWidget::ToggleContextMenu(const FInventoryItemHandle& ItemHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("[ToggleContextMenu] HandleId=%d, Current=%d"),
		ItemHandle.GetHandleId(), CurrentFocusedItemHandle.GetHandleId());

	if (!ContextMenuWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[ToggleContextMenu] FAILED: ContextMenuWidget is NULL"));
		return;
	}

	if (CurrentFocusedItemHandle == ItemHandle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToggleContextMenu] Same item -> clearing"));
		ClearContextMenu();
		return;
	}

	CurrentFocusedItemHandle = ItemHandle;

	UInventoryItemWidget** Found = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (!Found || !*Found)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToggleContextMenu] FAILED: No widget for HandleId=%d in PopulatedItemEntryWidgets (count=%d)"),
			ItemHandle.GetHandleId(), PopulatedItemEntryWidgets.Num());
		return;
	}

	UInventoryItemWidget* ItemWidget = *Found;
	SetContextMenuVisible(true);

	FVector2D ItemAbsPos = ItemWidget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D{ 1.f, 0.5f });
	FVector2D ItemWidgetPixelPos, ItemWidgetViewportPos;
	USlateBlueprintLibrary::AbsoluteToViewport(this, ItemAbsPos, ItemWidgetPixelPos, ItemWidgetViewportPos);

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (OwningPlayerController)
	{
		int ViewportSizeX, ViewportSizeY;
		OwningPlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
		int OverShoot = ItemWidgetPixelPos.Y + ContextMenuWidget->GetDesiredSize().Y * Scale - ViewportSizeY;
		if (OverShoot > 0)
		{
			ItemWidgetPixelPos.Y -= OverShoot;
		}
	}

	ContextMenuWidget->SetPositionInViewport(ItemWidgetPixelPos);
	UE_LOG(LogTemp, Log, TEXT("[ToggleContextMenu] Menu shown at (%f, %f)"), ItemAbsPos.X, ItemAbsPos.Y);
	/*	UInventoryItemWidget** ItemWidgetPtrPtr = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (!ItemWidgetPtrPtr)
		return;

	UInventoryItemWidget* ItemWidget = *ItemWidgetPtrPtr;
	if (!ItemWidget)
		return;

	SetContextMenuVisible(true);
	//UE_LOG(LogTemp, Warning, TEXT("You See???????"));
	FVector2D ItemAbsPos = ItemWidget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D{ 1.f,0.5f});
	
	FVector2D ItemWidgetPixelPos, ItemWidgetViewportPos;
	USlateBlueprintLibrary::AbsoluteToViewport(this, ItemAbsPos, ItemWidgetPixelPos, ItemWidgetViewportPos);

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (OwningPlayerController)
	{ 
		int ViewportSizeX, ViewportSizeY;
		OwningPlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		float Scale = UWidgetLayoutLibrary::GetViewportScale(this);

		int OverShoot = ItemWidgetPixelPos.Y + ContextMenuWidget->GetDesiredSize().Y * Scale - ViewportSizeY;
		if (OverShoot > 0)
		{
			ItemWidgetPixelPos.Y -= OverShoot;
		}
	}

	ContextMenuWidget->SetPositionInViewport(ItemAbsPos);
	//UE_LOG(LogTemp, Warning, TEXT("Trying to toggle context menu"));*/
}

void UInventoryWidget::ClearContextMenu()
{
	ContextMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	CurrentFocusedItemHandle = FInventoryItemHandle::InvalidHandle();
}

void UInventoryWidget::ItemAdded(const UInventoryItem* InventoryItem)
{
	if (!InventoryItem)
		return;

	if (UInventoryItemWidget* NextAvaliableSlot = GetNextAvaliableSlot())
	{
		NextAvaliableSlot->UpdateInventoryItem(InventoryItem);
		PopulatedItemEntryWidgets.Add(InventoryItem->GetHandle(), NextAvaliableSlot);
		if (InventoryComponent)
		{
			InventoryComponent->ItemSlotChanged(InventoryItem->GetHandle(), NextAvaliableSlot->GetSlotNumber());
		}
	}
}

void UInventoryWidget::ItemStackCountChanged(const FInventoryItemHandle& Handle, int Newcount)
{
	UInventoryItemWidget** FoundWidget = PopulatedItemEntryWidgets.Find(Handle);
	if (FoundWidget)
	{
		(*FoundWidget)->UpdateStackCount();
	}
}

UInventoryItemWidget* UInventoryWidget::GetNextAvaliableSlot() const
{
	for (UInventoryItemWidget* Widget : ItemWidgets)
	{
		if (Widget->IsEmpty())
		{
			return Widget;
		}
	}
	return nullptr;
}

void UInventoryWidget::HandleItemDragDrop(UInventoryItemWidget* DestinationWidget, UInventoryItemWidget* SourceWidget)
{
	const UInventoryItem* SrcItem = SourceWidget->GetInventoryItem();
	const UInventoryItem* DestinationItem = DestinationWidget->GetInventoryItem();

	DestinationWidget->UpdateInventoryItem(SrcItem);
	SourceWidget->UpdateInventoryItem(DestinationItem);

	PopulatedItemEntryWidgets[DestinationWidget->GetItemHandle()] = DestinationWidget;

	if (InventoryComponent)
	{
		InventoryComponent->ItemSlotChanged(DestinationWidget->GetItemHandle(), DestinationWidget->GetSlotNumber());
	}

	if (!SourceWidget->IsEmpty())
	{
		PopulatedItemEntryWidgets[SourceWidget->GetItemHandle()] = SourceWidget;
		if (InventoryComponent)
		{
			InventoryComponent->ItemSlotChanged(SourceWidget->GetItemHandle(), SourceWidget->GetSlotNumber());
		}
	}
}

void UInventoryWidget::ItemRemoved(const FInventoryItemHandle& ItemHandle)
{
	UInventoryItemWidget** FoundWidget = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (FoundWidget && *FoundWidget)
	{
		(*FoundWidget)->EmptySlot();
		PopulatedItemEntryWidgets.Remove(ItemHandle);
	}
}
