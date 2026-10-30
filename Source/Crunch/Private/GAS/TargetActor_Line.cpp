// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/TargetActor_Line.h"
#include "Crunch/Crunch.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Abilities/GameplayAbility.h"
#include "Kismet/KismetMathLibrary.h"

ATargetActor_Line::ATargetActor_Line()
{
	RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);

	TargetEndDetectionSphere = CreateDefaultSubobject<USphereComponent>("Target End Detection Sphere");
	TargetEndDetectionSphere->SetupAttachment(GetRootComponent());
	TargetEndDetectionSphere->SetCollisionResponseToChannel(ECC_SpringArm, ECR_Ignore);

	LazerVFX = CreateDefaultSubobject<UNiagaraComponent>("Lazer VFX");
	LazerVFX->SetupAttachment(GetRootComponent());

	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	ShouldProduceTargetDataOnServer = true;

	AvatarActor = nullptr;
}

void ATargetActor_Line::ConfigureTargetSetting(float NewTargetRange, float NewDetectionCyLinderRadius,
	float NewTargetingInterval, FGenericTeamId OwnerTeamId, bool bShouldDrawDebug)
{
	TargetRange = NewTargetRange;
	DetectionCylinderRadius = NewDetectionCyLinderRadius;
	TargetingInterval = NewTargetingInterval;
	SetGenericTeamId(OwnerTeamId);
	bDrawDebug = bShouldDrawDebug;
}

void ATargetActor_Line::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamId = NewTeamID;
}

void ATargetActor_Line::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATargetActor_Line, TeamId);
	DOREPLIFETIME(ATargetActor_Line, TargetRange);
	DOREPLIFETIME(ATargetActor_Line, DetectionCylinderRadius);
	DOREPLIFETIME(ATargetActor_Line, AvatarActor);
}

void ATargetActor_Line::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	if (!OwningAbility)
		return;
	AvatarActor = OwningAbility->GetAvatarActorFromActorInfo();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(PeoridicalTargetingTimerHandle, this, &ATargetActor_Line::DoTargetCheckAndReport, TargetingInterval, true);
	}
}

void ATargetActor_Line::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatedTargetTrace();
}

void ATargetActor_Line::BeginDestroy()
{
	if (GetWorld() && PeoridicalTargetingTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(PeoridicalTargetingTimerHandle);
	}

	Super::BeginDestroy();
}

void ATargetActor_Line::DoTargetCheckAndReport()
{
	if (!HasAuthority())
		return;

	TSet<AActor*> OverlappingActorSet;
	//取对象
	TargetEndDetectionSphere->GetOverlappingActors(OverlappingActorSet);

	TArray<TWeakObjectPtr<AActor>> OverlappingActors;
	//逐个过滤
	for (AActor* OverlappingActor : OverlappingActorSet)
	{
		if (ShouldReportActorAsTarget(OverlappingActor))
		{
			OverlappingActors.Add(OverlappingActor);
		}
	}
	FGameplayAbilityTargetDataHandle TargetDataHandle;
	//符合
	FGameplayAbilityTargetData_ActorArray* ActorArray = new FGameplayAbilityTargetData_ActorArray;
	ActorArray->SetActors(OverlappingActors);
	TargetDataHandle.Add(ActorArray);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}

void ATargetActor_Line::UpdatedTargetTrace()
{
	FVector ViewLocation = GetActorLocation();
	FRotator ViewRotation = GetActorRotation();
	if (AvatarActor)
	{
		AvatarActor->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}
	//很远的朝向终点
	FVector LookEndPoint = ViewLocation + ViewRotation.Vector() * 100000;
	//算朝向
	FRotator LookRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), LookEndPoint);
	SetActorRotation(LookRotation);

	//计算这次扫描的终点
	FVector SweepEndLocation = GetActorLocation() + LookRotation.Vector() * TargetRange;

	TArray<FHitResult> HitResults;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.AddIgnoredActor(this);

	FCollisionResponseParams CollisionResponseParams(ECR_Overlap);
	//做球形sweep
	GetWorld()->SweepMultiByChannel(HitResults, GetActorLocation(), SweepEndLocation, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(DetectionCylinderRadius), QueryParams, CollisionResponseParams);

	FVector LineEndLocation = SweepEndLocation;
	float LineLength = TargetRange;

	for (FHitResult& HitResult : HitResults)
	{
		if (GetTeamAttitudeTowards(*HitResult.GetActor()) != ETeamAttitude::Friendly)
		{
			LineEndLocation = HitResult.ImpactPoint;
			LineLength = FVector::Distance(GetActorLocation(), LineEndLocation);
			break;
		}
	}

	//末端检测球移到这个位置
	TargetEndDetectionSphere->SetWorldLocation(LineEndLocation);
	if (LazerVFX)
	{
		//更新
		LazerVFX->SetVariableFloat(LazerFXLengthParamName, LineLength / 100.f);
	}
}

bool ATargetActor_Line::ShouldReportActorAsTarget(const AActor* ActorToCheck) const
{
	if (!ActorToCheck)
		return false;

	if (ActorToCheck == AvatarActor)
		return false;

	if (ActorToCheck == this)
		return false;

	if (GetTeamAttitudeTowards(*ActorToCheck) != ETeamAttitude::Hostile)
		return false;

	return true;
}
