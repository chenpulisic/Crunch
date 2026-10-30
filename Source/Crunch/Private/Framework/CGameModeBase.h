// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericTeamAgentInterface.h"
#include "CGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class ACGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, const FString& Options) override;
	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* Controller) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

private:
	FGenericTeamId GetTeamIDForPlayer(const AController* InController) const;

	// Reserves the next unused PlayerStart with this team's tag.
	AActor* FindNextStartSpotForTeam(const FGenericTeamId& TeamID);

	// Weak keys: the GameMode can survive seamless travel, and we must not keep
	// PlayerStart actors from the old world alive through a strong UPROPERTY set.
	TSet<TWeakObjectPtr<AActor>> UsedStartSpots;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	TSubclassOf<APawn> BackupPawn;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	TMap<FGenericTeamId, FName> TeamStartSpotTagMap;

	class AStormCore* GetStormCore() const;

	void MatchFinished(AActor* ViewTarget, int WiningTeam);
};
