// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRHitStopSubsystem.h"

#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"

void UDDRHitStopSubsystem::RequestHitStop(int32 Frames)
{
	if (bHitStopActive || Frames <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Duration = static_cast<float>(Frames) / 60.f;
	FreezeEndRealTimeSeconds = FPlatformTime::Seconds() + Duration;
	bHitStopActive = true;
	UGameplayStatics::SetGlobalTimeDilation(World, 0.f);
}

void UDDRHitStopSubsystem::Tick(float DeltaTime)
{
	if (bHitStopActive && FPlatformTime::Seconds() >= FreezeEndRealTimeSeconds)
	{
		EndHitStop();
	}
}

TStatId UDDRHitStopSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDDRHitStopSubsystem, STATGROUP_Tickables);
}

bool UDDRHitStopSubsystem::IsTickable() const
{
	return bHitStopActive;
}

UWorld* UDDRHitStopSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void UDDRHitStopSubsystem::EndHitStop()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}

	bHitStopActive = false;
	FreezeEndRealTimeSeconds = 0.0;
}
