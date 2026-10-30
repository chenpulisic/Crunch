// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/MMC_LevelBased.h"
#include "GAS/CHeroAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CHeroAttributeSet.h"
#include "MMC_LevelBased.h"

UMMC_LevelBased::UMMC_LevelBased()
{
	LevelCaptureDefinition.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	LevelCaptureDefinition.AttributeToCapture = UCHeroAttributeSet::GetLevelAttribute();

	RelevantAttributesToCapture.Add(LevelCaptureDefinition);
}

float UMMC_LevelBased::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	UAbilitySystemComponent* ASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();
	if (!ASC)
		return 0.f;

	float Level = 0;
	FAggregatorEvaluateParameters EvaluateParams;
	EvaluateParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	GetCapturedAttributeMagnitude(LevelCaptureDefinition, Spec, EvaluateParams, Level);

	bool bFound;
	float RateAttributeVal = ASC->GetGameplayAttributeValue(RateAttribute, bFound);
	if (!bFound)
		return 0.f;

	return (Level - 1) * RateAttributeVal;
}
