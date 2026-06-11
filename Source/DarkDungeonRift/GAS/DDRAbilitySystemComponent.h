// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "DDRAbilitySystemComponent.generated.h"

UCLASS(ClassGroup = (DDR))
class DARKDUNGEONRIFT_API UDDRAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void AbilityLocalInputPressed(int32 InputID) override;
};
