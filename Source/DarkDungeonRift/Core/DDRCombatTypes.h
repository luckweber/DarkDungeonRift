// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRCombatTypes.generated.h"

/** O que o PLAYER faz durante um slam que derruba o alvo (ANS_DDRHitbox + bSlamDownTargets). */
UENUM(BlueprintType)
enum class EDDRSlamPlayerFollow : uint8
{
	/** Só derruba o alvo; player segue a física normal. */
	None UMETA(DisplayName = "None"),
	/** Congela o Z do player no ar durante o notify (slam-down no alvo; player cai depois). */
	PinInAir UMETA(DisplayName = "Pin In Air"),
	/** Player desce junto com o alvo (stocada / finisher até o chão). */
	FollowToGround UMETA(DisplayName = "Follow To Ground"),
};

USTRUCT(BlueprintType)
struct FDDRMeleeSweepParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SweepRadius = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SweepForwardOffset = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SweepReach = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 HitStopFrames = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 ComboSectionIndex = 0;

	// LANÇA os alvos atingidos (juggle entra): tag Airborne + sobe e segura (16 §2).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air")
	bool bLaunchTargets = false;

	// RE-FLUTUA alvos Airborne a cada hit (pop com decay, 16 §3). Use nos air attacks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air")
	bool bAirPop = false;

	// Arrasta alvos Airborne no XY junto com o atacante (set 06 — combo que avança no ar).
	// OFF no set 07 (Wave — player parado no ar). Liga por seção/notify na AM_AirCombo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air", meta = (EditCondition = "bAirPop"))
	bool bCarryAirborneTargets = false;

	/** Distância à frente do atacante onde o alvo juggleado fica (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air", meta = (EditCondition = "bCarryAirborneTargets", ClampMin = "0"))
	float AirCarryForwardOffset = 90.f;

	// Sweep vira AoE no dono (slam): COLUNA vertical — esfera de SweepRadius varrida do
	// dono para cima por SweepReach cm (alcança o alvo juggleado no alto). Ignora lâmina/forward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air")
	bool bAoEAtOwner = false;

	// Derruba alvos Airborne atingidos (queda rápida do slam, 16 §5).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air")
	bool bSlamDownTargets = false;

	/** Comportamento do player durante este slam (ver EDDRSlamPlayerFollow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air",
		meta = (EditCondition = "bSlamDownTargets"))
	EDDRSlamPlayerFollow SlamPlayerFollow = EDDRSlamPlayerFollow::PinInAir;

	/** Velocity Z do player em FollowToGround (cm/s, negativo = desce). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air",
		meta = (EditCondition = "bSlamDownTargets && SlamPlayerFollow == EDDRSlamPlayerFollow::FollowToGround", ClampMax = "0"))
	float SlamFollowFallVelocity = -3500.f;

	/** No 1º hit: pula a montage do slam pra secao End (ainda no ar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air",
		meta = (EditCondition = "bSlamDownTargets"))
	bool bJumpToSlamEndSection = false;

	/** Ao fim do notify com PinInAir: volta a cair e opcionalmente retoma a secao Loop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air",
		meta = (EditCondition = "bSlamDownTargets && SlamPlayerFollow == EDDRSlamPlayerFollow::PinInAir"))
	bool bResumeFallAfterSlamPin = true;
};
