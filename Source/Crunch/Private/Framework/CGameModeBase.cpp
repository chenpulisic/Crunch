// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameModeBase.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerStart.h"
#include "Framework/StormCore.h"
#include "Player/CPlayerController.h"
#include "Player/CPlayerState.h"

APlayerController* ACGameModeBase::SpawnPlayerController(ENetRole InRemoteRole, const FString& Options)
{
    APlayerController* NewPlayerController = Super::SpawnPlayerController(InRemoteRole, Options);
    IGenericTeamAgentInterface* NewPlayerTeamInterface = Cast<IGenericTeamAgentInterface>(NewPlayerController);
    FGenericTeamId TeamId = GetTeamIDForPlayer(NewPlayerController);
    if (NewPlayerTeamInterface)
    {
        NewPlayerTeamInterface->SetGenericTeamId(TeamId);
    }

    NewPlayerController->StartSpot = FindNextStartSpotForTeam(TeamId);

    return NewPlayerController;
}

void ACGameModeBase::StartPlay()
{
    Super::StartPlay();
    AStormCore* StormCore = GetStormCore();
    if (StormCore) 
    {
        StormCore->OnGoalReachedDelegate.AddUObject(this, &ACGameModeBase::MatchFinished);
    }
}

UClass* ACGameModeBase::GetDefaultPawnClassForController_Implementation(AController* Controller)
{
	ACPlayerState* CPlayerState = Controller->GetPlayerState<ACPlayerState>();
	if (CPlayerState && CPlayerState->GetSelectedPawnClass())
	{
		return CPlayerState->GetSelectedPawnClass();
	}
	return BackupPawn;
}

APawn* ACGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	IGenericTeamAgentInterface* NewPlayerTeamInterface = Cast<IGenericTeamAgentInterface>(NewPlayer);
	FGenericTeamId TeamId = GetTeamIDForPlayer(NewPlayer);
	if (NewPlayerTeamInterface)
	{
		NewPlayerTeamInterface->SetGenericTeamId(TeamId);
	}
	StartSpot = FindNextStartSpotForTeam(TeamId);
	NewPlayer->StartSpot = StartSpot;

	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot) ;
}

FGenericTeamId ACGameModeBase::GetTeamIDForPlayer(const AController* InController) const
{
	ACPlayerState* CPlayerState = InController->GetPlayerState<ACPlayerState>();
	if (CPlayerState && CPlayerState->GetSelectedPawnClass())
	{
		return CPlayerState->GetTeamIdBasedOnSlot();
	}

    static int PlayerCount = 0;
    ++PlayerCount;
    return FGenericTeamId(PlayerCount % 2);
}

AActor* ACGameModeBase::FindNextStartSpotForTeam(const FGenericTeamId& TeamID) const
{
    const FName* StartSpotTag = TeamStartSpotTagMap.Find(TeamID);
    if (!StartSpotTag)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();

    for(TActorIterator<APlayerStart> It(World); It; ++It)
    {
        if (It->PlayerStartTag == *StartSpotTag)
        {
            It->PlayerStartTag = FName("Taken");
            return *It;
        }
    }

    return nullptr;
}

AStormCore* ACGameModeBase::GetStormCore() const
{
    UWorld* World = GetWorld();
    if (World) 
    {
        for (TActorIterator<AStormCore> It(World); It; ++It)
        {
            return *It;
        }
    }

    return nullptr;
}

void ACGameModeBase::MatchFinished(AActor* ViewTarget, int WiningTeam)
{
	UWorld* World = GetWorld();
	if (World)
	{
        for (TActorIterator<ACPlayerController> It(World); It; ++It)
        {
            It->MatchFinished(ViewTarget, WiningTeam);
        }
	}
}
