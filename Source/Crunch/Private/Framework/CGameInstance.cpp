// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameInstance.h"
#include "Character/CCharacter.h"
#include "Framework/CGameState.h"
#include "Network/CNetStatics.h"

void UCGameInstance::StartMatch()
{
	if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer || GetWorld()->GetNetMode() == ENetMode::NM_ListenServer)
	{
		if (ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>())
		{
			CachePlayerSelections(CGameState->GetPlayerSelection());
			UE_LOG(LogTemp, Warning, TEXT("[StartMatch] Cached %d player selections before travel"), CachedPlayerSelections.Num());
		}

		LoadLevelAndListen(GameLevel);
	}
}

void UCGameInstance::CachePlayerSelections(const TArray<FPlayerSelection>& Selections)
{
	CachedPlayerSelections = Selections;
}

TSubclassOf<APawn> UCGameInstance::FindCachedPawnClass(const FString& PlayerName) const
{
	if (PlayerName.IsEmpty())
	{
		return nullptr;
	}

	for (const FPlayerSelection& Selection : CachedPlayerSelections)
	{
		if (Selection.GetPlayerNickName() != PlayerName)
		{
			continue;
		}

		if (UPA_CharacterDefinition* CharacterDefinition = Selection.GetCharacterDefinition().LoadSynchronous())
		{
			return CharacterDefinition->LoadCharacterClass();
		}
	}

	return nullptr;
}

FGenericTeamId UCGameInstance::FindCachedTeam(const FString& PlayerName) const
{
	if (PlayerName.IsEmpty())
	{
		return FGenericTeamId::NoTeam;
	}

	for (const FPlayerSelection& Selection : CachedPlayerSelections)
	{
		if (Selection.GetPlayerNickName() == PlayerName)
		{
			return FGenericTeamId(Selection.GetPlayerSlot() < UCNetStatics::GetPlayerCountPerTeam() ? 0 : 1);
		}
	}

	return FGenericTeamId::NoTeam;
}

void UCGameInstance::ClearCachedSelections()
{
	CachedPlayerSelections.Reset();
}

void UCGameInstance::LoadLevelAndListen(TSoftObjectPtr<UWorld> Level)
{
	const FName LevelURL = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));
	if (LevelURL != "")
	{
		GetWorld()->ServerTravel(LevelURL.ToString() + "?listen");
	}
}
