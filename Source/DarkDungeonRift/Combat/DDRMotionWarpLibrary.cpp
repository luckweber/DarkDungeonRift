// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRMotionWarpLibrary.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"

namespace DDRMotionWarp
{
	FVector ComputeWarpPoint(
		const FVector& OwnerLocation,
		const FVector& TargetLocation,
		const float IdealHitDistance,
		const float MaxWarpDistance,
		const bool bIncludeZ)
	{
		FVector ToTarget = TargetLocation - OwnerLocation;
		if (!bIncludeZ)
		{
			ToTarget.Z = 0.f;
		}

		const float Dist = ToTarget.Size();
		if (Dist <= KINDA_SMALL_NUMBER || Dist > MaxWarpDistance)
		{
			return FVector::ZeroVector;
		}

		const FVector Dir = ToTarget / Dist;
		FVector WarpPoint = TargetLocation - Dir * IdealHitDistance;
		if (!bIncludeZ)
		{
			WarpPoint.Z = OwnerLocation.Z;
		}

		return WarpPoint;
	}

	bool ApplyAttackWarp(
		UMotionWarpingComponent* MotionWarping,
		AActor* TargetActor,
		const EDDRMotionWarpProfile Profile,
		const float IdealHitDistance,
		const float MaxWarpDistance,
		const float MaxWarpDistanceAir,
		const float MaxWarpDistanceLauncher)
	{
		if (!MotionWarping || !TargetActor || Profile == EDDRMotionWarpProfile::None)
		{
			return false;
		}

		ACharacter* OwnerChar = MotionWarping->GetCharacterOwner();
		if (!OwnerChar)
		{
			return false;
		}

		const FVector OwnerLoc = OwnerChar->GetActorLocation();
		const FVector TargetLoc = TargetActor->GetActorLocation();

		float MaxDist = MaxWarpDistance;
		float IdealDist = IdealHitDistance;
		bool bIncludeZ = false;

		switch (Profile)
		{
		case EDDRMotionWarpProfile::Ground:
		case EDDRMotionWarpProfile::RunAttack:
			MaxDist = MaxWarpDistance;
			IdealDist = IdealHitDistance;
			bIncludeZ = false;
			break;

		case EDDRMotionWarpProfile::Air:
			MaxDist = MaxWarpDistanceAir;
			IdealDist = IdealHitDistance * 0.5f;
			bIncludeZ = false;
			break;

		case EDDRMotionWarpProfile::Launcher:
			MaxDist = MaxWarpDistanceLauncher;
			IdealDist = IdealHitDistance;
			bIncludeZ = false; // subida vem do root motion do clip
			break;

		case EDDRMotionWarpProfile::Slam:
			// Só encara o alvo — sem deslocamento horizontal (descida é velocity).
			{
				FVector To = TargetLoc - OwnerLoc;
				To.Z = 0.f;
				if (!To.IsNearlyZero())
				{
					const FRotator FaceRot(0.f, To.Rotation().Yaw, 0.f);
					MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
						DDRMotionWarpNames::AttackWarp, OwnerLoc, FaceRot);
				}
			}
			return true;

		default:
			return false;
		}

		const FVector WarpPoint = ComputeWarpPoint(OwnerLoc, TargetLoc, IdealDist, MaxDist, bIncludeZ);
		if (WarpPoint.IsNearlyZero())
		{
			MotionWarping->RemoveWarpTarget(DDRMotionWarpNames::AttackWarp);
			return false;
		}

		FVector To = TargetLoc - OwnerLoc;
		To.Z = 0.f;
		const FRotator FaceRot = To.IsNearlyZero() ? OwnerChar->GetActorRotation() : FRotator(0.f, To.Rotation().Yaw, 0.f);

		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
			DDRMotionWarpNames::AttackWarp, WarpPoint, FaceRot);

#if ENABLE_DRAW_DEBUG
		if (UWorld* World = OwnerChar->GetWorld())
		{
			DrawDebugSphere(World, WarpPoint, 20.f, 10, FColor::Magenta, false, 0.5f);
			DrawDebugLine(World, OwnerLoc, WarpPoint, FColor::Magenta, false, 0.5f, 0, 2.f);
		}
#endif

		return true;
	}

	void ClearAttackWarp(UMotionWarpingComponent* MotionWarping)
	{
		if (MotionWarping)
		{
			MotionWarping->RemoveWarpTarget(DDRMotionWarpNames::AttackWarp);
		}
	}
}
