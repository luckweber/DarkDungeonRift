// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DDREnemyAIController.generated.h"

class ADDREncounterManager;
class ADDREnemyCharacter;
class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class DARKDUNGEONRIFT_API ADDREnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ADDREnemyAIController();

	UFUNCTION(BlueprintCallable, Category = "DDR|AI")
	void SetAggroTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "DDR|AI|Token")
	void ReleaseAttackToken();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	void UpdateBlackboard(float DeltaSeconds);
	void RunBehaviorTreeForEnemy(ADDREnemyCharacter* Enemy);

	UPROPERTY(EditDefaultsOnly, Category = "DDR|AI", meta = (ClampMin = "100"))
	float AggroRadius = 5000.f;

	UPROPERTY()
	TObjectPtr<UBlackboardComponent> DDRBlackboard = nullptr;
};
