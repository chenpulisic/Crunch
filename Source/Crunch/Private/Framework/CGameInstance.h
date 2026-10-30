// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "Player/PlayerInfoTypes.h"
#include "CGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class UCGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	void StartMatch();

	void CachePlayerSelections(const TArray<FPlayerSelection>& Selections);
	TSubclassOf<APawn> FindCachedPawnClass(const FString& PlayerName) const;
	FGenericTeamId FindCachedTeam(const FString& PlayerName) const;
	void ClearCachedSelections();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> GameLevel;

	// Hard travel（PIE 默认）会重建 PlayerState，选人结果先缓存在 GameInstance 里。
	// Seamless travel 下 PlayerState 会自己带过去，这个缓存只是兜底。
	UPROPERTY()
	TArray<FPlayerSelection> CachedPlayerSelections;

	void LoadLevelAndListen(TSoftObjectPtr<UWorld> Level);
};
