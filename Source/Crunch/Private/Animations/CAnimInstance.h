// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "CAnimInstance.generated.h"

/**
 * 
 */



UCLASS()
class UCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DelataSconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DelataSconds) override;

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float GetSpeed() const { return Speed; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE bool IsMoving() const { return Speed != 0; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE bool IsNotMoving() const { return Speed == 0; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float GetYawSpeed() const { return YawSpeed; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float GetFwdSpeed() const { return FwdSpeed; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float GetRightSpeed() const { return RightSpeed; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float GetSmoothedYawSpeed() const { return SmoothedYawSpeed; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE bool IsJumping() const { return Jumping; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE bool IsGround() const { return !Jumping; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float LookOffsetYaw() const { return LookRotOffSet.Yaw; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE float LookOffsetPtich() const { return LookRotOffSet.Pitch; }

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
FORCEINLINE bool GetIsAimming() const {return bISAimming;}

UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
bool ShouldDoFullBody() const;
private:
	void OwnerAimTagChanged(const FGameplayTag Tag, int32 NewCount);
	UPROPERTY()
	class ACharacter* OwnerCharacter;
	UPROPERTY()
	class UCharacterMovementComponent* OwnerMoveMent;
	UPROPERTY(EditAnywhere, Category = "Animation")
	float YawSpeedSmoothLerpSpeed = 1.f;

	float Speed;
	float SmoothedYawSpeed;
	float YawSpeed;
	float RightSpeed;
	float FwdSpeed;
	FRotator BodyPrevRot;
	bool Jumping;
	bool bISAimming;
	FRotator LookRotOffSet;
};
