// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DDRHitStopSubsystem.generated.h"

UCLASS()
class DARKDUNGEONRIFT_API UDDRHitStopSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RequestHitStop(int32 Frames);

private:
	void EndHitStop();

	FTimerHandle HitStopTimerHandle;
	bool bHitStopActive = false;
};
