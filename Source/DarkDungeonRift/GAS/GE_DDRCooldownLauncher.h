// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_DDRCooldownLauncher.generated.h"

UCLASS()
class DARKDUNGEONRIFT_API UGE_DDRCooldownLauncher : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_DDRCooldownLauncher();

protected:
	virtual void PostInitProperties() override;
};
