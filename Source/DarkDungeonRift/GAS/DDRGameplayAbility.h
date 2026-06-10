// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "DDRGameplayAbility.generated.h"

class UDDRAbilitySystemComponent;
class UDDRCombatComponent;

UCLASS(Abstract)
class DARKDUNGEONRIFT_API UDDRGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UDDRGameplayAbility();

	UFUNCTION(BlueprintPure, Category = "DDR|Ability")
	UDDRAbilitySystemComponent* GetDDRAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "DDR|Ability")
	UDDRCombatComponent* GetDDRCombatComponent() const;
};
