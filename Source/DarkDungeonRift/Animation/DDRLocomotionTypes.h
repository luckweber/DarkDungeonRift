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

/** Direção do pulo de combate relativa ao facing (doc 59). */
UENUM(BlueprintType)
enum class EDDRJumpDirection : uint8
{
	Neutral,
	Forward,
	Back,
	Left,
	Right
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

	/** Direção do takeoff (Start / Double anim). */
	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	EDDRJumpDirection JumpDirection = EDDRJumpDirection::Neutral;

	/** Direção do pouso (End anim) — último pulo antes do land. */
	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	EDDRJumpDirection LandDirection = EDDRJumpDirection::Neutral;

	/** Pulso de 1 frame: usar SÓ na TRANSIÇÃO Fall Loop → Jump (re-entra no estado).
	 *  NÃO use no conteúdo do estado Jump — quando o estado avalia, o pulso já morreu. */
	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bIsDoubleJump = false;

	/** PERSISTENTE: o pulo atual foi iniciado no AR (double jump OU queda de borda).
	 *  É ESTE que o conteúdo do estado Jump usa (Blend Poses by Bool → Double_Jump_Combat_*),
	 *  porque sobrevive o estado inteiro. Sem ele, o pulso bIsDoubleJump some antes do
	 *  Blend avaliar e a anim de double nunca toca (doc 59 §4.1). Reset no pouso. */
	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	bool bJumpFromAir = false;

	/** 0=chão · 1=1º pulo · 2=double (debug/tuning). */
	UPROPERTY(BlueprintReadOnly, Category = "DDR|Locomotion")
	int32 AirJumpIndex = 0;
};
