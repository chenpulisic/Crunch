// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ItemWidget.h"
#include "Components/Image.h"
#include "Widgets/ItemToolTip.h"

void UItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UItemWidget::SetIcon(UTexture2D* IconTexture)
{
	ItemIcon->SetBrushFromTexture(IconTexture);
}

UItemToolTip* UItemWidget::SetToolTipWidget(const UPA_ShopItem* Item)
{
	if (!Item)
		return nullptr;

	if (GetOwningPlayer() && ItemToolTipClass)
	{
		UItemToolTip* ToolTip = CreateWidget<UItemToolTip>(GetOwningPlayer(), ItemToolTipClass);
		if (ToolTip)
		{
			ToolTip->SetItem(Item);
			SetToolTip(ToolTip);
		}

		return ToolTip;
	}

	return nullptr;
}

FReply UItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply SuperReply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		return FReply::Handled().SetUserFocus(TakeWidget());
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().SetUserFocus(TakeWidget()).DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return SuperReply;
}

FReply UItemWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply SuperReply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	//if (HasAnyUserFocus())
	//{
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemWidget] RightMouseButton UP -> RightButtonClicked() on %s"), *GetClass()->GetName());
			RightButtonClicked();
			UE_LOG(LogTemp, Warning, TEXT("[ItemWidget] RightButtonClicked() returned"));
			return FReply::Handled();
		}
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemWidget] LeftMouseButton UP -> LeftButtonClicked()"));
			LeftButtonClicked();
			return FReply::Handled();
		}
	//}
	/*else
	{
		// 失去了用户焦点 → 右键事件不会触发!
		UE_LOG(LogTemp, Warning, TEXT("[ItemWidget] MouseButton UP but NO USER FOCUS! Button=%s, Class=%s"),
			InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton ? TEXT("RIGHT") :
			InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ? TEXT("LEFT") : TEXT("OTHER"),
			*GetClass()->GetName());
	}*/

	return SuperReply;
}

void UItemWidget::RightButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("&&&Right mouse button clicked"));
}

void UItemWidget::LeftButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("&&&Left mouse button clicked"));
}
