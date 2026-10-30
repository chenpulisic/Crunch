// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GAP_Launched.h"
#include "GAS/CAbilitySystemStatics.h"

UGAP_Launched::UGAP_Launched()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = GetLaunchedAbilityActiatinTag();

	ActivationBlockedTags.RemoveTag(UCAbilitySystemStatics::GetStunStatTag());
	AbilityTriggers.Add(TriggerData);
}

void UGAP_Launched::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if(!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	if (K2_HasAuthority())
	{
		PushSelf(TriggerEventData->TargetData.Get(0)->GetHitResult()->ImpactNormal);
		const FHitResult* HitResultPtr = TriggerEventData->TargetData.Get(0)->GetHitResult();
		if (HitResultPtr)
		{
			AActor* HitActor = HitResultPtr->GetActor();
			if (HitActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("±»Åö×²ActorÃû³Æ£º%s"), *HitActor->GetName());
			}
		}
		K2_EndAbility();
	}
}

FGameplayTag UGAP_Launched::GetLaunchedAbilityActiatinTag()
{
	return FGameplayTag::RequestGameplayTag("ability.passive.launch.active");
}
