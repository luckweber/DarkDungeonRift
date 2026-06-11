// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRMotionWarpTypes.generated.h"

/** Nome do warp target nas montages (AnimNotifyState Motion Warping → Warp Target Name). */
namespace DDRMotionWarpNames
{
	inline const FName AttackWarp = TEXT("AttackWarp");
}

/**
 * Perfil de motion warp por verbo de ataque (doc 18 §6.3).
 * Cada ability escolhe o perfil; a montage precisa de janela Motion Warping com target "AttackWarp".
 */
UENUM(BlueprintType)
enum class EDDRMotionWarpProfile : uint8
{
	None UMETA(DisplayName = "None"),
	Ground UMETA(DisplayName = "Ground Combo"),
	RunAttack UMETA(DisplayName = "Run Attack"),
	Air UMETA(DisplayName = "Air Combo"),
	Launcher UMETA(DisplayName = "Launcher"),
	Slam UMETA(DisplayName = "Slam (face only)"),
};
