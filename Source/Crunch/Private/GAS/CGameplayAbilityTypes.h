// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffect.h"
#include "CGameplayAbilityTypes.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECAbilityInputID : uint8
{
	None              UMETA(DisplayName="None"),
	BasicAttack       UMETA(DisplayName="BasicAttack"),
	Aim               UMETA(DisplayName="Aim"),
	AbilityOne        UMETA(DisplayName="AbilityOne"),
	AbilityTwo        UMETA(DisplayName="AbilityTwo"),
	AbilityThree      UMETA(DisplayName="AbilityThree"),
	AbilityFour       UMETA(DisplayName="AbilityFour"),
	AbilityFive       UMETA(DisplayName="AbilityFive"),
	AbilitySix        UMETA(DisplayName="AbilitySix"),
	Confirm           UMETA(DisplayName="Confirm"),
	Cancel            UMETA(DisplayName="Cancel")
};

USTRUCT(BlueprintType)
struct FGenericDamageEffectDef
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere)
	FVector PushVelocity;
};
USTRUCT(BlueprintType)
struct FHeroBaseStats : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Class;

	UPROPERTY(EditAnywhere)
	float Strength;

	UPROPERTY(EditAnywhere)
	float Intelligence;

	UPROPERTY(EditAnywhere)
	float StrengthGrowthRate;

	UPROPERTY(EditAnywhere)
	float IntelligenceGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere)
	float BaseMaxMana;

	UPROPERTY(EditAnywhere)
	float BaseAttackDamage;

	UPROPERTY(EditAnywhere)
	float BaseArmor;

	UPROPERTY(EditAnywhere)
	float BaseMoveSpeed;

	UPROPERTY(EditAnywhere)
	float BaseMoveAcceleration;
};