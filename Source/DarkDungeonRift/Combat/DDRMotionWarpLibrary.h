// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRMotionWarpTypes.h"

class AActor;
class UMotionWarpingComponent;

namespace DDRMotionWarp
{
	/** Calcula o ponto de warp clampado por MaxWarpDist (doc 18 §6.3). */
	DARKDUNGEONRIFT_API FVector ComputeWarpPoint(
		const FVector& OwnerLocation,
		const FVector& TargetLocation,
		float IdealHitDistance,
		float MaxWarpDistance,
		bool bIncludeZ);

	/** Aplica warp target "AttackWarp" no MotionWarpingComponent. Retorna false se não warpa (whiff honesto). */
	DARKDUNGEONRIFT_API bool ApplyAttackWarp(
		UMotionWarpingComponent* MotionWarping,
		AActor* TargetActor,
		EDDRMotionWarpProfile Profile,
		float IdealHitDistance,
		float MaxWarpDistance,
		float MaxWarpDistanceAir,
		float MaxWarpDistanceLauncher);

	DARKDUNGEONRIFT_API void ClearAttackWarp(UMotionWarpingComponent* MotionWarping);
}
