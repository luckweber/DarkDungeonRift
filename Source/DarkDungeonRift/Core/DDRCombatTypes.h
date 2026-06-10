// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRCombatTypes.generated.h"

USTRUCT(BlueprintType)
struct FDDRMeleeSweepParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SweepRadius = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SweepForwardOffset = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SweepReach = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 HitStopFrames = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 ComboSectionIndex = 0;
};
