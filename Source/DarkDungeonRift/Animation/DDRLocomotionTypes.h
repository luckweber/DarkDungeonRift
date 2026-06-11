// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRLocomotionTypes.generated.h"

UENUM(BlueprintType)
enum class EDDRGait : uint8
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class EDDRMovementMode : uint8
{
	Grounded,
	InAir,
	Combat
};

/** Shared locomotion snapshot — written by CMC/AnimInstance (doc 08 §2.2). */
USTRUCT(BlueprintType)
struct FDDRLocomotionState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	EDDRGait Gait = EDDRGait::Run;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bWantsToStop = false;

	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bIsDashing = false;

	/** true durante slam descendente — AnimBP: Jump_Combat_Loop / Jump_Combat_End. */
	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bIsCombatFalling = false;
};
