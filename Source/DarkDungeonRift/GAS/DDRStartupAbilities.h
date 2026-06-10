// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "DDRAbilityInput.h"
#include "DDRStartupAbilities.generated.h"

USTRUCT(BlueprintType)
struct FDDRCStartupAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ability")
	TSubclassOf<UGameplayAbility> Ability;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ability")
	EDDRAbilityInputID InputID = EDDRAbilityInputID::None;
};
