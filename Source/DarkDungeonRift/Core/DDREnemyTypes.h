// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDREnemyTypes.generated.h"

class UDDREnemyData;

/** Papel tático do inimigo (doc 31). */
UENUM(BlueprintType)
enum class EDDREnemyRole : uint8
{
	Grunt UMETA(DisplayName = "Grunt"),
	Ranged UMETA(DisplayName = "Ranged"),
	Elite UMETA(DisplayName = "Elite"),
	Boss UMETA(DisplayName = "Boss"),
};

/** Estados de debug/transição da IA (doc 30 §3). */
UENUM(BlueprintType)
enum class EDDREnemyAIState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack"),
	Reposition UMETA(DisplayName = "Reposition"),
};

/** Telegrafe no chão (doc 32 §1 / doc 61 §7). */
USTRUCT(BlueprintType)
struct FDDRTelegraphConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	float WindupSeconds = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	float ConeAngleDegrees = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	float ConeRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	FLinearColor Color = FLinearColor(1.f, 0.1f, 0.1f, 0.5f);
};

/** Blackboard keys canônicas (doc 61 §3). */
namespace DDRBlackboardKeys
{
	inline const FName TargetActor(TEXT("TargetActor"));
	inline const FName AIState(TEXT("AIState"));
	inline const FName bInRange(TEXT("bInRange"));
	inline const FName bHasAttackToken(TEXT("bHasAttackToken"));
	inline const FName DesiredRange(TEXT("DesiredRange"));
	inline const FName bPlayerTooClose(TEXT("bPlayerTooClose"));
}

USTRUCT(BlueprintType)
struct FDDRSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UDDREnemyData> EnemyData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FDDRWave
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave")
	TArray<FDDRSpawnEntry> Spawns;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	float DelayBeforeSeconds = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1"))
	int32 MaxAlive = 8;
};
