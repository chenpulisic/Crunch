// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PA_CharacterDefinition.h"
#include "PlayerInfoTypes.generated.h"

class APlayerState;
class UPA_CharacterDefination;
USTRUCT()
struct FPlayerSelection
{
	GENERATED_BODY()
public:
	FPlayerSelection();
	FPlayerSelection(uint8 InSlot, const APlayerState* InPlayerState);

	FORCEINLINE void SetSlot(uint8 NewSlot) { Slot = NewSlot; }
	FORCEINLINE uint8 GetPlayerSlot() const { return Slot; }
	FORCEINLINE FUniqueNetIdRepl GetPlayerUniqueNetId() const { return PlayerUniqueId; }
	FORCEINLINE FString GetPlayerNickName() const { return PlayerNickName; }
	FORCEINLINE TSoftObjectPtr<UPA_CharacterDefinition> GetCharacterDefinition() const { return CharacterDefinition; }
	FORCEINLINE void SetCharacterDefinition(TSoftObjectPtr<UPA_CharacterDefinition> NewCharacterDefinition) { CharacterDefinition = NewCharacterDefinition; }

	bool IsForPlayer(const APlayerState* PlayerState) const;
	bool IsValid() const;


	static uint8 GetInvalidSlot();
private:
	UPROPERTY()
	uint8 Slot;

	UPROPERTY()
	FUniqueNetIdRepl PlayerUniqueId;

	UPROPERTY()
	FString PlayerNickName;

	UPROPERTY()
	 TSoftObjectPtr<UPA_CharacterDefinition> CharacterDefinition;
};
