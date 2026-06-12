// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDREnemyAIController.h"

#include "DDREncounterManager.h"
#include "DDREnemyCharacter.h"
#include "DDREnemyData.h"
#include "DDREnemyTypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

ADDREnemyAIController::ADDREnemyAIController()
{
	bWantsPlayerState = false;
}

void ADDREnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ADDREnemyCharacter* Enemy = Cast<ADDREnemyCharacter>(InPawn))
	{
		RunBehaviorTreeForEnemy(Enemy);
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		SetAggroTarget(PlayerPawn);
	}
}

void ADDREnemyAIController::OnUnPossess()
{
	ReleaseAttackToken();
	Super::OnUnPossess();
}

void ADDREnemyAIController::RunBehaviorTreeForEnemy(ADDREnemyCharacter* Enemy)
{
	if (!Enemy || !Enemy->GetEnemyData())
	{
		return;
	}

	if (UBehaviorTree* BT = Enemy->GetEnemyData()->BehaviorTree.LoadSynchronous())
	{
		RunBehaviorTree(BT);
	}
	else
	{
		UE_LOG(LogDDR, Warning, TEXT("[AI] %s sem BehaviorTree no DataAsset."), *GetNameSafe(Enemy));
	}
}

void ADDREnemyAIController::SetAggroTarget(AActor* NewTarget)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(DDRBlackboardKeys::TargetActor, NewTarget);
	}
}

void ADDREnemyAIController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateBlackboard(DeltaSeconds);
}

void ADDREnemyAIController::UpdateBlackboard(const float DeltaSeconds)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	ADDREnemyCharacter* Enemy = Cast<ADDREnemyCharacter>(GetPawn());
	if (!BB || !Enemy)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(DDRTags::State_Combat_Airborne)
			|| ASC->HasMatchingGameplayTag(DDRTags::State_Combat_Stagger)
			|| ASC->HasMatchingGameplayTag(DDRTags::State_Dead))
		{
			BB->SetValueAsBool(DDRBlackboardKeys::bHasAttackToken, false);
			return;
		}
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(DDRBlackboardKeys::TargetActor));
	if (!Target)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			Target = PlayerPawn;
			BB->SetValueAsObject(DDRBlackboardKeys::TargetActor, Target);
		}
	}

	if (!Target)
	{
		return;
	}

	const float Dist2D = FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation());
	const bool bRanged = Enemy->GetEnemyData() && Enemy->GetEnemyData()->Role == EDDREnemyRole::Ranged;

	if (bRanged)
	{
		BB->SetValueAsFloat(DDRBlackboardKeys::DesiredRange, Enemy->GetDesiredCombatRange());
		BB->SetValueAsBool(DDRBlackboardKeys::bPlayerTooClose, Dist2D < Enemy->GetMinRangedRange());
		BB->SetValueAsBool(DDRBlackboardKeys::bInRange,
			Dist2D >= Enemy->GetMinRangedRange() && Dist2D <= Enemy->GetDesiredCombatRange() + 100.f);
	}
	else
	{
		BB->SetValueAsBool(DDRBlackboardKeys::bInRange, Dist2D <= Enemy->GetMeleeAttackRange());
	}

	ADDREncounterManager* Encounter = ADDREncounterManager::FindForWorld(GetWorld());
	const bool bWantsToken = BB->GetValueAsBool(DDRBlackboardKeys::bInRange);
	bool bHasToken = Encounter && Encounter->HasAttackToken(Enemy);
	if (bWantsToken && Encounter && !bHasToken)
	{
		bHasToken = Encounter->TryGrantAttackToken(Enemy);
	}

	BB->SetValueAsBool(DDRBlackboardKeys::bHasAttackToken, bHasToken);
}

void ADDREnemyAIController::ReleaseAttackToken()
{
	if (ADDREnemyCharacter* Enemy = Cast<ADDREnemyCharacter>(GetPawn()))
	{
		if (ADDREncounterManager* Encounter = ADDREncounterManager::FindForWorld(GetWorld()))
		{
			Encounter->ReleaseAttackToken(Enemy);
		}
	}
}
