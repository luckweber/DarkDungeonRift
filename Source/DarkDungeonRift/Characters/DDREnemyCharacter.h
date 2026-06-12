// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRCharacterBase.h"
#include "DDREnemyCharacter.generated.h"

class UDDREnemyData;

UCLASS()
class DARKDUNGEONRIFT_API ADDREnemyCharacter : public ADDRCharacterBase
{
	GENERATED_BODY()

public:
	ADDREnemyCharacter();

	UFUNCTION(BlueprintCallable, Category = "DDR|Enemy")
	UDDREnemyData* GetEnemyData() const { return EnemyData; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Enemy")
	void ApplyEnemyData(UDDREnemyData* Data);

	UFUNCTION(BlueprintPure, Category = "DDR|Enemy")
	float GetMeleeAttackRange() const { return MeleeAttackRange; }

	UFUNCTION(BlueprintPure, Category = "DDR|Enemy")
	float GetDesiredCombatRange() const { return DesiredCombatRange; }

	UFUNCTION(BlueprintPure, Category = "DDR|Enemy")
	float GetMinRangedRange() const { return MinRangedRange; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DDR|Enemy")
	TObjectPtr<UDDREnemyData> EnemyData = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "DDR|Enemy")
	float MeleeAttackRange = 180.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "DDR|Enemy")
	float DesiredCombatRange = 750.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "DDR|Enemy")
	float MinRangedRange = 500.f;
};
