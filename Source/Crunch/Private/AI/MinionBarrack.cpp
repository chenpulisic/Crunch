// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/MinionBarrack.h"
#include "GameFramework/PlayerStart.h"
#include "AI/Minion.h"

// Sets default values
AMinionBarrack::AMinionBarrack()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMinionBarrack::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(SpawnIntervalTimerHandle, this, &AMinionBarrack::SpawnNewGroup, GroupSpawnInterval,true);
	}
}

// Called every frame
void AMinionBarrack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

const APlayerStart* AMinionBarrack::GetNextSpawnSpot()
{
	if (SpawnSpots.Num() == 0)
	{
		return nullptr;
	}

	++NextSpwanSpotIndex;

	if (NextSpwanSpotIndex >= SpawnSpots.Num())
	{
		NextSpwanSpotIndex = 0;
	}
	
	return SpawnSpots[NextSpwanSpotIndex];
}

void AMinionBarrack::SpawnNewMinions(int Amt)
{
	for (int i = 0; i < Amt; i++)
	{
		FTransform SpawnTransform = GetActorTransform();
		if (const APlayerStart* NextSpwanSpot = GetNextSpawnSpot())
		{
			SpawnTransform = NextSpwanSpot->GetActorTransform();
		}

		AMinion* NewMinion = GetWorld()->SpawnActorDeferred<AMinion>(MinionClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		NewMinion->SetGenericTeamId(BarrackTeamId);
		NewMinion->FinishSpawning(SpawnTransform);
		NewMinion->SetGoal(Goal);
		MininonPool.Add(NewMinion);
	}
}

AMinion* AMinionBarrack::GetNextAvaliableMinion() const
{
	for(AMinion * Minion : MininonPool)
	{
		if (!Minion->IsActive())
		{
			return Minion;
		}
	}
	return nullptr;
}

void AMinionBarrack::SpawnNewGroup()
{
	int i = MinionPerGroup;

	while (i > 0)
	{
		FTransform SpawnTransform = GetActorTransform();
		if (const APlayerStart* NextSpwanSpot = GetNextSpawnSpot())
		{
			SpawnTransform = NextSpwanSpot->GetActorTransform();
		}
	
		AMinion* NextAvaliableMinion = GetNextAvaliableMinion();
		if(!NextAvaliableMinion)
		{
			break;
		}

		NextAvaliableMinion->SetActorTransform(SpawnTransform);
		NextAvaliableMinion->Activate();
		--i;
	}
	SpawnNewMinions(i);
}