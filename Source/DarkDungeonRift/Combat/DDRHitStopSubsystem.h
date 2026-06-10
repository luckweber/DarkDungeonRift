// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "DDRHitStopSubsystem.generated.h"

UCLASS()
class DARKDUNGEONRIFT_API UDDRHitStopSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void RequestHitStop(int32 Frames);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

private:
	void EndHitStop();

	bool bHitStopActive = false;
	double FreezeEndRealTimeSeconds = 0.0;
};
