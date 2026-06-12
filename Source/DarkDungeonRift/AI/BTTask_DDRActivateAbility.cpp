// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_DDRActivateAbility.h"

#include "DDREnemyAIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "DDRLog.h"
#include "TimerManager.h"

struct FBTActivateAbilityMemory
{
	FDelegateHandle AbilityEndedHandle;
	FTimerHandle TimeoutHandle;
};

UBTTask_DDRActivateAbility::UBTTask_DDRActivateAbility()
{
	bNotifyTaskFinished = true;
}

uint16 UBTTask_DDRActivateAbility::GetInstanceMemorySize() const
{
	return sizeof(FBTActivateAbilityMemory);
}

EBTNodeResult::Type UBTTask_DDRActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTActivateAbilityMemory* Memory = reinterpret_cast<FBTActivateAbilityMemory*>(NodeMemory);
	Memory->AbilityEndedHandle.Reset();
	Memory->TimeoutHandle.Invalidate();

	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC || !AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	FGameplayTagContainer Tags;
	Tags.AddTag(AbilityTag);

	if (!ASC->TryActivateAbilitiesByTag(Tags))
	{
		UE_LOG(LogDDR, Verbose, TEXT("[BT] TryActivateAbilitiesByTag falhou: %s"), *AbilityTag.ToString());
		return EBTNodeResult::Failed;
	}

	if (!bWaitForAbilityEnd)
	{
		return EBTNodeResult::Succeeded;
	}

	const FGameplayTag WaitTag = AbilityTag;
	Memory->AbilityEndedHandle = ASC->OnAbilityEnded.AddLambda(
		[this, &OwnerComp, WaitTag](const FAbilityEndedData& EndedData)
		{
			if (EndedData.AbilityThatEnded && EndedData.AbilityThatEnded->AbilityTags.HasTag(WaitTag))
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
		});

	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().SetTimer(
			Memory->TimeoutHandle,
			FTimerDelegate::CreateLambda([&OwnerComp, this]()
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}),
			FailTimeoutSeconds,
			false);
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_DDRActivateAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UBTTask_DDRActivateAbility::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const EBTNodeResult::Type TaskResult)
{
	FBTActivateAbilityMemory* Memory = reinterpret_cast<FBTActivateAbilityMemory*>(NodeMemory);
	if (APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
		{
			if (Memory->AbilityEndedHandle.IsValid())
			{
				ASC->OnAbilityEnded.Remove(Memory->AbilityEndedHandle);
			}
		}
	}

	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().ClearTimer(Memory->TimeoutHandle);
	}

	if (ADDREnemyAIController* AI = Cast<ADDREnemyAIController>(OwnerComp.GetAIOwner()))
	{
		AI->ReleaseAttackToken();
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
