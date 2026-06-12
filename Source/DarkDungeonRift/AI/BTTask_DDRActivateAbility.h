// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_DDRActivateAbility.generated.h"

UCLASS()
class DARKDUNGEONRIFT_API UBTTask_DDRActivateAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_DDRActivateAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "DDR|Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "DDR|Ability")
	bool bWaitForAbilityEnd = true;

	UPROPERTY(EditAnywhere, Category = "DDR|Ability", meta = (ClampMin = "0.1"))
	float FailTimeoutSeconds = 8.f;
};
