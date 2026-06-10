// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_DDRCooldownDash.generated.h"

UCLASS()
class DARKDUNGEONRIFT_API UGE_DDRCooldownDash : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_DDRCooldownDash();

protected:
	virtual void PostInitProperties() override;
};
