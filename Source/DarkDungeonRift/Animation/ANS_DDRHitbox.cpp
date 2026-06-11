// Copyright Epic Games, Inc. All Rights Reserved.

#include "ANS_DDRHitbox.h"

#include "DDRCombatComponent.h"

void UANS_DDRHitbox::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	if (UDDRCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UDDRCombatComponent>())
	{
		Combat->ResetHitTracking();
		if (SweepParams.bCarryAirborneTargets)
		{
			Combat->SetAirCarryActive(true, SweepParams.AirCarryForwardOffset);
		}
		if (SweepParams.bSlamDownTargets)
		{
			Combat->SetSlamPinSweepParams(SweepParams);
			if (SweepParams.SlamPlayerFollow == EDDRSlamPlayerFollow::PinInAir)
			{
				Combat->BeginSlamAirPin();
			}
		}
	}
}

void UANS_DDRHitbox::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	if (UDDRCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UDDRCombatComponent>())
	{
		FDDRMeleeSweepParams Params = SweepParams;
		Combat->PerformMeleeSweep(Params);
		if (Params.bCarryAirborneTargets)
		{
			Combat->SyncJuggleTargetFollow();
		}
	}
}

void UANS_DDRHitbox::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	if (UDDRCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UDDRCombatComponent>())
	{
		if (SweepParams.bSlamDownTargets
			&& SweepParams.SlamPlayerFollow == EDDRSlamPlayerFollow::PinInAir)
		{
			Combat->EndSlamAirPin(SweepParams);
		}
	}
}
