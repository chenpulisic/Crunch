// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Blackhole.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GAS/TargetActor_GroundPick.h"
#include "GAS/TA_Blackhole.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"

void UGA_Blackhole::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		K2_EndAbility();
		return;
	}
	//UE_LOG(LogTemp, Warning, TEXT("[BH] ActivateAbility called, HasAuth=%d, PredKeyValid=%d"),CurrentActorInfo->IsNetAuthority(),CurrentActivationInfo.GetPredictionKey().IsValid());
	PlayCastBlackholeMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargettingMontage);
	PlayCastBlackholeMontageTask->OnBlendOut.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->OnCancelled.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->OnInterrupted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->OnCompleted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitPlacementTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitPlacementTask->ValidData.AddDynamic(this, &UGA_Blackhole::PlaceBlackhole);
	WaitPlacementTask->Cancelled.AddDynamic(this, &UGA_Blackhole::PlacementCancelled);
	WaitPlacementTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitPlacementTask->BeginSpawningActor(this, TargetActorClass, TargetActor);
	ATargetActor_GroundPick* GroundPickTargetActor = Cast<ATargetActor_GroundPick>(TargetActor);
	if (GroundPickTargetActor)
	{
		GroundPickTargetActor->SetShouldDrawDebug(ShouldDrawDebug());
		GroundPickTargetActor->SetTargetAreaRadius(TargetAreaRadius);
		GroundPickTargetActor->SetTargetTraceRange(TargetTraceRange);
	}
	WaitPlacementTask->FinishSpawningActor(this, TargetActor);
	AddAimEffect();
}

void UGA_Blackhole::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveAimEffect();
	RemoveFocusEffect();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Blackhole::PlaceBlackhole(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("[BH] PlaceBlackhole called"));

	if (!K2_CommitAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BH] CommitAbility FAILED"));
		K2_EndAbility();
		return;
	}

	RemoveAimEffect();
	AddFocusEffect();

	if (PlayCastBlackholeMontageTask)
	{
		PlayCastBlackholeMontageTask->OnBlendOut.RemoveAll(this);
		PlayCastBlackholeMontageTask->OnCancelled.RemoveAll(this);
		PlayCastBlackholeMontageTask->OnInterrupted.RemoveAll(this);
		PlayCastBlackholeMontageTask->OnCompleted.RemoveAll(this);
	}

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BH] Auth/PredKey passed, playing HoldMontage"));
		UAbilityTask_PlayMontageAndWait* PlayHoleBlackholeMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, HoldBlackholeMontage);
		PlayHoleBlackholeMontage->OnBlendOut.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoleBlackholeMontage->OnCancelled.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoleBlackholeMontage->OnInterrupted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoleBlackholeMontage->OnCompleted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoleBlackholeMontage->ReadyForActivation();
	}

	BlackholeTargettingTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, BlackholeTargetActorClass);
	BlackholeTargettingTask->ValidData.AddDynamic(this, &UGA_Blackhole::FinalTargetsReceived);
	BlackholeTargettingTask->Cancelled.AddDynamic(this, &UGA_Blackhole::FinalTargetsReceived);
	BlackholeTargettingTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	BlackholeTargettingTask->BeginSpawningActor(this, BlackholeTargetActorClass, TargetActor);
	ATA_Blackhole* BlackholeTargetActor = Cast<ATA_Blackhole>(TargetActor);
	UE_LOG(LogTemp, Warning, TEXT("[BH] BlackholeTargetActor cast: %s"), BlackholeTargetActor ? TEXT("OK") : TEXT("NULL"));
	if (BlackholeTargetActor)
	{
		BlackholeTargetActor->ConfigureBlackhole(TargetAreaRadius, BlackholePullSpeed, BlackholeDuration, GetOwnerTeamId());
	}
	BlackholeTargettingTask->FinishSpawningActor(this, TargetActor);
	if (BlackholeTargetActor)
	{
		FVector BlackholeLocation = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
		BlackholeTargetActor->SetActorLocation(BlackholeLocation);
		UE_LOG(LogTemp, Warning, TEXT("[BH] Setting BH location: %s"), *BlackholeLocation.ToString());
	}

}

void UGA_Blackhole::PlacementCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_Blackhole::FinalTargetsReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (K2_HasAuthority())
	{
		BP_ApplyGameplayEffectToTarget(TargetDataHandle, FinalBlowDamageEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
		FVector BlowCenter = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
		PushTargetsFromLocation(TargetDataHandle, BlowCenter, BlowPushSpeed);

		UAbilityTask_PlayMontageAndWait* PlayFinalBlowMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FinalBlowMontage);
		PlayFinalBlowMontage->OnBlendOut.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->OnCancelled.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->OnInterrupted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->OnCompleted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->ReadyForActivation();
	}
	else
	{
		PlayMontageLocally(FinalBlowMontage);
	}

	FGameplayCueParameters FinalBlowCueParameters;
	FinalBlowCueParameters.Location = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
	FinalBlowCueParameters.RawMagnitude = TargetAreaRadius;

	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(FinalBlowCueTag, FinalBlowCueParameters);
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(UCAbilitySystemStatics::GetCameraShakeGameplayCueTag(), FinalBlowCueParameters);
}

void UGA_Blackhole::AddAimEffect()
{
	AimEffectHandle = BP_ApplyGameplayEffectToOwner(AimEffect);
}

void UGA_Blackhole::RemoveAimEffect()
{
	if (AimEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(AimEffectHandle);
	}
}

void UGA_Blackhole::AddFocusEffect()
{
	FocusEffectHandle = BP_ApplyGameplayEffectToOwner(FocusEffect);
}

void UGA_Blackhole::RemoveFocusEffect()
{
	if (FocusEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(FocusEffectHandle);
	}
}

