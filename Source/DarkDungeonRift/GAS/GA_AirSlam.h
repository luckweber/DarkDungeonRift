// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_AirSlam.generated.h"

class UAnimMontage;

// Finisher descendente (doc 16 par.5), 3 FASES: "Start" (mergulho) -> "Loop" (queda,
// self-loop via Montage_SetNextSection — dura o que a queda durar) -> "End" (impacto,
// pulado pelo LandedDelegate). A descida REAL e o codigo (velocity -3500); o AoE do
// pouso e uma COLUNA vertical (raio x altura) — pega o alvo juggleado la no alto e
// derruba (bSlamDownTargets). Secao "Loop" e opcional (montage 2-secoes funciona).
UCLASS()
class DARKDUNGEONRIFT_API UGA_AirSlam : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AirSlam();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION()
	void OnSlamLanded(const FHitResult& Hit);

	UFUNCTION()
	void OnMontageFinished();

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	TObjectPtr<UAnimMontage> SlamMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	FName StartSection = TEXT("Start");

	/** Secao de queda (opcional): self-loop enquanto desce; o pouso pula pra End. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	FName LoopSection = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	FName LandSection = TEXT("End");

	/** Velocity Z da descida — aplicada SEMPRE na ativação (mesmo se o slam cortar o
	 *  launcher no meio da montage, quando o hold aéreo ainda nem começou). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam", meta = (ClampMax = "0"))
	float SlamFallVelocity = -3500.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	float SlamDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	float SlamRadius = 250.f;

	/** Altura da COLUNA do AoE acima do ponto de pouso (cm) — alcanca o alvo juggleado. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam", meta = (ClampMin = "0"))
	float SlamVerticalReach = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	int32 SlamHitStopFrames = 6;

	// Coreografia DMC: o golpe do APEX (inicio do Start) ja ARREMESSA o alvo juggleado —
	// ele cai a SlammedFallVelocity (-4500, mais rapido que o player) e esmaga no chao
	// PRIMEIRO; o AoE do pouso cobre o dano + outros Airborne no raio.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Feel")
	bool bSlamClaimedTargetOnActivate = true;

	/** Hit-stop do "agarrao" no apex (frames) — vende o golpe que arremessa. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Feel", meta = (ClampMin = "0", ClampMax = "10"))
	int32 SlamGrabHitStopFrames = 2;

	// Homing por VELOCITY (motion warp nao serve aqui: warp e root motion, e a descida
	// e velocity sem RM): desce mirando o XY do alvo do soft-lock, com cap honesto.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Homing")
	bool bHomeToTarget = true;

	/** Acima desta distancia XY (cm) NAO persegue — desce reto (whiff honesto). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Homing", meta = (ClampMin = "0"))
	float SlamHomingMaxDistance = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Homing", meta = (ClampMin = "0"))
	float SlamMaxHorizontalSpeed = 1800.f;

private:
	bool bLandedBound = false;
	// Garante AoE/cue uma unica vez e decide quem encerra a ability (End ou pouso tardio).
	bool bImpactTriggered = false;

	// RootMotionMode do AnimInstance e forcado pra IgnoreRootMotion durante o slam
	// (RM ON num clip de queda "dirigiria" a capsula e mataria a velocity -3500).
	uint8 SavedRootMotionMode = 0;
	bool bSavedRootMotionMode = false;
};
