// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRHitStopSubsystem.h"

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

	bHitStopActive = true;
	UGameplayStatics::SetGlobalTimeDilation(World, 0.01f);

	const float Duration = static_cast<float>(Frames) / 60.f;
	World->GetTimerManager().SetTimer(
		HitStopTimerHandle,
		this,
		&UDDRHitStopSubsystem::EndHitStop,
		Duration,
		false);
}

void UDDRHitStopSubsystem::EndHitStop()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}

	bHitStopActive = false;
}
