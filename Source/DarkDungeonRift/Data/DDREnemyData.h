// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDREnemyTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DDREnemyData.generated.h"

class UBehaviorTree;
class UGameplayAbility;
class USkeletalMesh;

UCLASS(BlueprintType)
class DARKDUNGEONRIFT_API UDDREnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FGameplayTag EnemyId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	EDDREnemyRole Role = EDDREnemyRole::Grunt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	float Health = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float PoiseMax = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float PoiseRegenPerSecond = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float PoiseRegenDelaySeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float MoveSpeed = 420.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.1"))
	float Mass = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float Armor = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float AttackPower = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 EssenceDrop = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TSoftObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FDDRTelegraphConfig Telegraph;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/** Alcance melee para bInRange (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "50"))
	float MeleeAttackRange = 180.f;

	/** Distância ideal ranged (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "100"))
	float DesiredCombatRange = 750.f;

	/** Abaixo disto o ranged tenta kite (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "50"))
	float MinRangedRange = 500.f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
