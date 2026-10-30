// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetWork.h"
#include "Crunch/Crunch.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/CAbilitySystemComponent.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Widgets/OverHeadStatsGauge.h"

// Sets default values
ACCharacter::ACCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_SpringArm, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Target, ECR_Ignore);

	CAbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>("CAbility System Component");
	CAttributeSet = CreateDefaultSubobject<UCAttributeSet>("CAttribute Set");
	OverHeadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("Over Head Widget Component");
	OverHeadWidgetComponent->SetupAttachment(GetRootComponent());
	
	BindGASChangeDelegates();

	PerceptionStimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("Perception Stimuli Source Component");
}

void ACCharacter::ServeSideInit()
{
	UE_LOG(LogTemp, Warning, TEXT("[ServeSideInit] ===== %s ====="), *GetName());
	UE_LOG(LogTemp, Warning, TEXT("[ServeSideInit] Class=%s, Role=%d, HasAuthority=%d"),
		*GetClass()->GetName(),
		(int32)GetLocalRole(),
		HasAuthority());

	if (!CAbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[ServeSideInit] ABORTED: CAbilitySystemComponent is NULL!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ServeSideInit] -> InitAbilityActorInfo(this=%s, this)"),
		*GetName());
	CAbilitySystemComponent->InitAbilityActorInfo(this, this);

	UE_LOG(LogTemp, Warning, TEXT("[ServeSideInit] -> ServerSideInit()"));
	CAbilitySystemComponent->ServerSideInit();

	UE_LOG(LogTemp, Warning, TEXT("[ServeSideInit] ===== DONE %s ====="), *GetName());
	/*CAbilitySystemComponent->InitAbilityActorInfo(this, this);
	CAbilitySystemComponent->ServerSideInit();*/
}

void ACCharacter::ClientSideInit()
{
	UE_LOG(LogTemp, Warning, TEXT("[ClientSideInit] %s -> InitAbilityActorInfo(this, this)"), *GetName());
	CAbilitySystemComponent->InitAbilityActorInfo(this, this);
}

bool ACCharacter::IsLocallyControlledByPlayer() const
{
	return GetController() != nullptr && GetController()->IsLocalController();
}

void ACCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogTemp, Warning, TEXT("[PossessedBy] %s, Controller=%s, IsLocal=%d, IsPlayer=%d, NetMode=%d"),
		*GetName(),
		*GetNameSafe(NewController),
		NewController ? NewController->IsLocalController() : -1,
		NewController ? NewController->IsPlayerController() : -1,
		(int32)GetNetMode());

	//   核心修复：HasAuthority() 是判断"是否为服务端"的正确API
	//    IsLocalController() 在 Listen Server 下对 AIController 返回 true（引擎行为差异）
	//    导致 Minion 的 ServeSideInit 被错误跳过
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PossessedBy] -> Calling ServeSideInit() for %s (HasAuthority=TRUE)"),
			*GetName());
		ServeSideInit();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PossessedBy] ServeSideInit SKIPPED for %s (HasAuthority=FALSE, client-side)"),
			*GetName());
	}
	/*Super::PossessedBy(NewController);
	if (NewController && !NewController->IsLocalController())
	{
		ServeSideInit();
	}*/
}

void ACCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCharacter, TeamID);
}

// Called when the game starts or when spawned
void ACCharacter::BeginPlay()
{
	Super::BeginPlay();
	ConfigureOverHeadStatusWidget();
	MeshRealativeTransform = GetMesh()->GetRelativeTransform();
	PerceptionStimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
}

// Called every frame
void ACCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

UAbilitySystemComponent* ACCharacter::GetAbilitySystemComponent() const
{
	return CAbilitySystemComponent;
}

void ACCharacter::Server_SendGameplayEventToSelf_Implementation(AActor* TargetActor, const FGameplayTag& EventTag,
	const FGameplayEventData& EventData)
{
	if(!IsValid(TargetActor))
	{
		return;
	}
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, EventData);
}

bool ACCharacter::Server_SendGameplayEventToSelf_Validate(AActor* TargetActor, const FGameplayTag& EventTag,
	const FGameplayEventData& EventData)
{
	return IsValid(TargetActor);
}

void ACCharacter::UpgradeAbilityWithInputID(ECAbilityInputID InputID)
{
	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->Server_UpgradeAbilityWithID(InputID);
	}
}

void ACCharacter::BindGASChangeDelegates()
{
	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatTag()).AddUObject(this, &ACCharacter::DeathTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatTag()).AddUObject(this, &ACCharacter::StunTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetAimStatTag()).AddUObject(this, &ACCharacter::AimTagUpdated);
		CAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetFocusStatTag()).AddUObject(this, &ACCharacter::FocusTagUpdated);

		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ACCharacter::MoveSpeedUpdated);
		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ACCharacter::MaxHealthUpdated);
		CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMaxManaAttribute()).AddUObject(this, &ACCharacter::MaxManaUpdated);
		//CAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetMoveAccelerationAttribute()).AddUObject(this, &ACCharacter::MoveSpeedAccelerationUpdated);
	
	}
}

void ACCharacter::DeathTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount != 0)
	{
		StartDeathSquence();
	}
	else
	{
		Respawn();
	}
}

void ACCharacter::StunTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if(IsDead()) return;
	
	if (NewCount != 0)
	{
		OnStun();
		PlayAnimMontage(StunMontage);
	}
	
	else
	{
		OnRecoverFromStun();
		StopAnimMontage(StunMontage);
	}
}

void ACCharacter::AimTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	SetIsAimming(NewCount != 0);
}

void ACCharacter::FocusTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	bIsInFocusMode = NewCount > 0;
}

void ACCharacter::SetIsAimming(bool bIsAimming)
{
	bUseControllerRotationYaw = bIsAimming;
	GetCharacterMovement()->bOrientRotationToMovement = !bIsAimming;
	OnAimStateChanged(bIsAimming);
}

void ACCharacter::MoveSpeedUpdated(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void ACCharacter::OnAimStateChanged(bool bIsAimming)
{
	//Override in child class
}

void ACCharacter::MaxHealthUpdated(const FOnAttributeChangeData& Data)
{
	if (IsValid(CAttributeSet))
	{
		CAttributeSet->RescaleHealth();
	}
}

void ACCharacter::MaxManaUpdated(const FOnAttributeChangeData& Data)
{
	if (IsValid(CAttributeSet))
	{
		CAttributeSet->RescaleMana();
	}
}

void ACCharacter::ConfigureOverHeadStatusWidget()
{
	if (!OverHeadWidgetComponent)
	{
		return;
	}

	if(IsLocallyControlledByPlayer())
	{
		OverHeadWidgetComponent->SetHiddenInGame(true);
		return;
	}

	UOverHeadStatsGauge* OverheadStatsGauge = Cast<UOverHeadStatsGauge>(OverHeadWidgetComponent->GetUserWidgetObject());
	if (OverheadStatsGauge)
	{
		OverheadStatsGauge->ConfigureWithASC(GetAbilitySystemComponent());
		OverHeadWidgetComponent->SetHiddenInGame(false);
		GetWorldTimerManager().ClearTimer(HeadStatGaugeVisibilityUpdateTimerHandle);
		GetWorldTimerManager().SetTimer(HeadStatGaugeVisibilityUpdateTimerHandle, this, &ACCharacter::UpdateHeadGaugeVisibity, HeadStatGaugeVisibilityCheckUpdateGap,true);
	}
}

void ACCharacter::UpdateHeadGaugeVisibity()
{
	APawn* LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (LocalPlayerPawn)
	{
		float DistSquared = FVector::DistSquared(GetActorLocation(), LocalPlayerPawn->GetActorLocation());
		OverHeadWidgetComponent->SetHiddenInGame(DistSquared > HeadStatGaugeVisibilityRangeSquard);
	}
}

void ACCharacter::DeathMontageFinished()
{
	if (IsDead())
	{
		SetRagdollEnabled(true);
	}
}

void ACCharacter::SetRagdollEnabled(bool bIsEnabled)
{
	if (bIsEnabled)
	{
		GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
	else
	{
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		GetMesh()->SetRelativeTransform(MeshRealativeTransform);
	}
}

void ACCharacter::PlayDeathAnimation()
{
	if (DeathMontage)
	{
		float MontageDuration = PlayAnimMontage(DeathMontage);
		GetWorldTimerManager().SetTimer(DeathMontageTimerHandle, this, &ACCharacter::DeathMontageFinished, MontageDuration + DeathMontageFinishTimeShift);
	}
}

void ACCharacter::SetStatusGaugeEnabled(bool bIsEnabled)
{
	GetWorldTimerManager().ClearTimer(HeadStatGaugeVisibilityUpdateTimerHandle);
	if (bIsEnabled)
	{
		ConfigureOverHeadStatusWidget();
	}
	else
	{
		OverHeadWidgetComponent->SetHiddenInGame(true);
	}
}

bool ACCharacter::IsDead() const
{
	return GetAbilitySystemComponent()->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDeadStatTag());
}

void ACCharacter::RespawnImmediately()
{
	if (HasAuthority())
	{
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(UCAbilitySystemStatics::GetDeadStatTag()));
	}
}

void ACCharacter::StartDeathSquence()
{
	OnDead();

	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->CancelAbilities();
	}

	PlayDeathAnimation();
	SetStatusGaugeEnabled(false);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetAIPerceptionStimuliSourceEnabled(false);
}

void ACCharacter::Respawn()
{
	OnRespawn();
	SetRagdollEnabled(false);
	SetAIPerceptionStimuliSourceEnabled(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	GetMesh()->GetAnimInstance()->StopAllMontages(0.f);
	SetStatusGaugeEnabled(true);

	if(HasAuthority() && GetController())
	{
		TWeakObjectPtr<AActor> StartSpot = GetController()->StartSpot;
		if (StartSpot.IsValid())
		{
			SetActorTransform(StartSpot->GetActorTransform());
		}
	}

	if (CAbilitySystemComponent)
	{
		CAbilitySystemComponent->ApplyFullStatEffect();
	}
}

void ACCharacter::OnDead()
{ 
}

void ACCharacter::OnRespawn()
{
}

void ACCharacter::OnStun()
{
}

void ACCharacter::OnRecoverFromStun()
{

}

void ACCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
}

FGenericTeamId ACCharacter::GetGenericTeamId() const
{
	return TeamID;
}

bool ACCharacter::IsEqualTeamID(FGenericTeamId TeamID1, FGenericTeamId TeamID2)
{
	return TeamID1 != TeamID2;
}

void ACCharacter::OnRep_TeamID()
{
	//override in childclass
}

void ACCharacter::SetAIPerceptionStimuliSourceEnabled(bool bIsEnabled)
{
	if (!PerceptionStimuliSourceComponent)
	{
		return;
	}
	if (bIsEnabled)
	{
		PerceptionStimuliSourceComponent->RegisterWithPerceptionSystem();
	}
	else
	{
		PerceptionStimuliSourceComponent->UnregisterFromPerceptionSystem();
	}
}

const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>&  ACCharacter::GetAbilities() const
{
	return CAbilitySystemComponent->GetAbilities();
}

FVector ACCharacter::GetCaptureLocalPosition() const
{
	return HeadshotCaptureLocalPosition;
}

FRotator ACCharacter::GetCaptureLocalRotation() const
{
	return HeadshotCaptureLocalRotation;
}
