// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryItem.h"
#include "AbilitySystemComponent.h"
#include "Inventory/PA_ShopItem.h"
#include "GameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"

FInventoryItemHandle::FInventoryItemHandle()
	:HandleId{GetInvalidId()}
{
}

FInventoryItemHandle::FInventoryItemHandle(uint32 Id)
	:HandleId{ Id }
{
}


FInventoryItemHandle FInventoryItemHandle::InvalidHandle()
{
	static FInventoryItemHandle InvalidHandle = FInventoryItemHandle();
	return InvalidHandle;
}

FInventoryItemHandle FInventoryItemHandle::CreateHandle()
{
	return FInventoryItemHandle(GenerateNextId());
}

uint32 FInventoryItemHandle::GenerateNextId()
{
	static uint32 StaticId = 1;
	return StaticId++;
}

uint32 FInventoryItemHandle::GetInvalidId()
{
	return 0;
}

bool FInventoryItemHandle::IsValid() const
{
	return HandleId != GetInvalidId();
}

bool operator==(const FInventoryItemHandle& Lhs, const FInventoryItemHandle& Rhs)
{
	return Lhs.GetHandleId() == Rhs.GetHandleId();
}

uint32 GetTypeHash(const FInventoryItemHandle& Key)
{
	return Key.GetHandleId();
}

void UInventoryItem::InitItem(const FInventoryItemHandle& NewHandle, const UPA_ShopItem* NewShopItem)
{
	Handle = NewHandle;
	ShopItem = NewShopItem;
}

bool UInventoryItem::TryActivateGrantedAbility(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!GrantedAbilitySpecHandle.IsValid())
		return false;

	if (AbilitySystemComponent && AbilitySystemComponent->TryActivateAbility(GrantedAbilitySpecHandle))
		return true;

	return false;
}

void UInventoryItem::ApplyConsumeEffect(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!ShopItem)
		return;
	TSubclassOf<UGameplayEffect> ConsumeEffect = ShopItem->GetConsumeEffect();
	if (!ConsumeEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyConsumeEffect] ConsumeEffect Class NULL!"));
		return;
	}

	// 强制确认：这行逻辑必须运行在服务器！
	UE_LOG(LogTemp, Warning, TEXT("[ApplyConsumeEffect] OwnerRole:%d, GE Class:%s"),
		(int)AbilitySystemComponent->GetOwner()->GetLocalRole(), *ConsumeEffect->GetName());

	// 拿到CDO对象
	UGameplayEffect* EffectCDO = ConsumeEffect->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this); //补全上下文信息，很多GE/ExecCalc依赖SourceObject

	FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(ConsumeEffect, 1, Context);

	UE_LOG(LogTemp, Warning, TEXT("[ApplyConsumeEffect] Apply Return Handle.IsValid():%d"), ActiveHandle.IsValid());
	/*if (!ShopItem)
		return;

	TSubclassOf<UGameplayEffect> ConsumeEffect = ShopItem->GetConsumeEffect();
	if (!ConsumeEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("&&[ApplyConsumeEffect]"));
		return;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyConsumeEffect]"));
	}
	AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(ConsumeEffect, 1, AbilitySystemComponent->MakeEffectContext());*/
}

void UInventoryItem::RemoveGASModifications(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
		return;

	if (AppliedEquipedEffectHandle.IsValid())
		AbilitySystemComponent->RemoveActiveGameplayEffect(AppliedEquipedEffectHandle);

	if (GrantedAbilitySpecHandle.IsValid())
		AbilitySystemComponent->SetRemoveAbilityOnEnd(GrantedAbilitySpecHandle);
}


void UInventoryItem::ApplyGASModifications(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!GetShopItem() || !AbilitySystemComponent)
	{
		return;
	}

	if (!AbilitySystemComponent->GetOwner() || !AbilitySystemComponent->GetOwner()->HasAuthority())
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EquipEffect = GetShopItem()->GetEquippedEffect();
	if (EquipEffect)
	{
		AppliedEquipedEffectHandle = AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(EquipEffect, 1, AbilitySystemComponent->MakeEffectContext());
	}

	TSubclassOf<UGameplayAbility> GrantedAbility = GetShopItem()->GetGrantedAbility();
	if (GrantedAbility)
	{
		const FGameplayAbilitySpec* FoundSpec = AbilitySystemComponent->FindAbilitySpecFromClass(GrantedAbility);
		if (FoundSpec)
		{
			GrantedAbilitySpecHandle = FoundSpec->Handle;
		}
		else
		{
			GrantedAbilitySpecHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(GrantedAbility));
		}
	}

}

void UInventoryItem::SetSlot(int NewSlot)
{
	Slot = NewSlot;
}

bool UInventoryItem::AddStackCount()
{
	if (IsStackFull())
	{
		return false;
	}

	++StackCount;
	return true;
}

bool UInventoryItem::ReduceStackCount()
{
	--StackCount;
	if (StackCount <= 0)
	{
		return false;
	}
	return true;
}

bool UInventoryItem::SetStackCount(int NewStackCount)
{
	if (NewStackCount > 0 && NewStackCount <= GetShopItem()->GetMaxStackCount())
	{
		StackCount = NewStackCount;
		return true;
	}
	return false;
}

bool UInventoryItem::IsStackFull() const
{
	return StackCount >= GetShopItem()->GetMaxStackCount();
}

bool UInventoryItem::IsForItem(const UPA_ShopItem* Item) const
{
	if (!Item)
		return false;

	return GetShopItem() == Item;
}

UInventoryItem::UInventoryItem()
	:StackCount{1}
{
}

bool UInventoryItem::IsValid() const
{
	return ShopItem != nullptr;
}
