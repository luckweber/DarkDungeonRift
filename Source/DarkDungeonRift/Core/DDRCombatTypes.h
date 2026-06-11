// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRCombatTypes.generated.h"

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

	// Sweep vira AoE esférico centrado no dono (slam): ignora lâmina/forward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air")
	bool bAoEAtOwner = false;

	// Derruba alvos Airborne atingidos (queda rápida do slam, 16 §5).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Air")
	bool bSlamDownTargets = false;
};
