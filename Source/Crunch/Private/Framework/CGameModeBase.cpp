// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameModeBase.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerStart.h"
#include "Framework/CGameInstance.h"
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

    // 不在这里消耗 PlayerStart：此时 PlayerState 可能还没就绪，
    // 选人/队伍信息也不完整。等 SpawnDefaultPawnFor 里再按最终 TeamId 选。
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

	// Hard travel（PIE 默认）会重建 PlayerState，选人结果从 GameInstance 的缓存里取。
	if (CPlayerState)
	{
		if (UCGameInstance* CGameInstance = Cast<UCGameInstance>(GetGameInstance()))
		{
			if (TSubclassOf<APawn> CachedPawnClass = CGameInstance->FindCachedPawnClass(CPlayerState->GetPlayerName()))
			{
				UE_LOG(LogTemp, Warning, TEXT("[PawnClass] Cached %s for %s"),
					*GetNameSafe(CachedPawnClass), *CPlayerState->GetPlayerName());
				return CachedPawnClass;
			}
		}
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

	// 优先复用 Controller 上已经分配好的出生点；没有的话只选一次。
	// 这样不会在 SpawnPlayerController 和这里各消耗一次 PlayerStart。
	AActor* TeamSpot = NewPlayer->StartSpot.Get();
	if (!IsValid(TeamSpot))
	{
		TeamSpot = FindNextStartSpotForTeam(TeamId);
	}
	if (IsValid(TeamSpot))
	{
		StartSpot = TeamSpot;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnDefaultPawnFor] No team start for %s (Team=%d), fallback to any PlayerStart"),
			*GetNameSafe(NewPlayer), TeamId.GetId());
	}

	// 再兜底：RestartPlayer 传进来的点，或者地图里任意一个 PlayerStart。
	if (!IsValid(StartSpot))
	{
		StartSpot = FindPlayerStart(NewPlayer);
	}

	// AGameModeBase::SpawnDefaultPawnFor_Implementation 内部直接调用
	// StartSpot->GetActorRotation()，不会判空；nullptr 会在这里崩。
	if (!IsValid(StartSpot))
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnDefaultPawnFor] No valid PlayerStart for %s (Team=%d)"),
			*GetNameSafe(NewPlayer), TeamId.GetId());
		return nullptr;
	}

	NewPlayer->StartSpot = StartSpot;
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

FGenericTeamId ACGameModeBase::GetTeamIDForPlayer(const AController* InController) const
{
	ACPlayerState* CPlayerState = InController->GetPlayerState<ACPlayerState>();
	if (CPlayerState && CPlayerState->GetSelectedPawnClass())
	{
		return CPlayerState->GetTeamIdBasedOnSlot();
	}

	// Hard travel 下 PlayerState 的 PlayerSelection 会丢，从 GameInstance 的缓存取队伍。
	if (CPlayerState)
	{
		if (UCGameInstance* CGameInstance = Cast<UCGameInstance>(GetGameInstance()))
		{
			FGenericTeamId CachedTeam = CGameInstance->FindCachedTeam(CPlayerState->GetPlayerName());
			if (CachedTeam != FGenericTeamId::NoTeam)
			{
				return CachedTeam;
			}
		}
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
