// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRAbilityInput.generated.h"

UENUM(BlueprintType)
enum class EDDRAbilityInputID : uint8
{
	None = 0,
	Attack,
	Heavy,
	Dash,
	Launcher,
	AirSlam,
	Parry,
	MAX UMETA(Hidden)
};
