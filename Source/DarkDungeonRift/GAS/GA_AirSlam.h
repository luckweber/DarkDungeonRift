// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_AirSlam.generated.h"

class UAnimMontage;

/** Quando a montage pula pra secao End (impacto / slam-down no ar). */
UENUM(BlueprintType)
enum class EDDRSlamEndTrigger : uint8
{
	/** Pouso no chao -> secao End (finisher com player no solo). */
	OnGroundLand UMETA(DisplayName = "On Ground Land"),
	/** 1º hit com bJumpToSlamEndSection no hitbox (ainda no ar). */
	OnSlamHit UMETA(DisplayName = "On Slam Hit"),
	/** Ainda caindo, a X cm do chao -> End (slam-down aereo antes do pouso). */
	BeforeGroundProximity UMETA(DisplayName = "Before Ground Proximity"),
};

// Finisher descendente (doc 16 par.5), 3 FASES: "Start" (mergulho/apex) -> "Loop" (queda,
// self-loop) -> "End" (impacto no chao, pulado pelo LandedDelegate). Descida = velocity.
// Dano + derrube do alvo = ANS_DDRHitbox nas secoes Start/End (bSlamDownTargets) —
// NAO no pouso nem na ativacao (a espada tem que acertar).
UCLASS()
class DARKDUNGEONRIFT_API UGA_AirSlam : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AirSlam();

	/** Chamado pelo CombatComponent (hitbox / pin). */
	void JumpToMontageSection(FName SectionName);
	void NotifySlamHitboxJumpToEnd();
	void ResumeFallAfterSlamPin(float FallVelocityZ, bool bResumeLoopSection);
	bool IsEndSectionStarted() const { return bEndSectionStarted; }

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

	/** Valores sugeridos pro ANS_DDRHitbox na secao End (dano real vem do notify). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|HitboxDefaults", meta = (DisplayName = "Suggested Damage (doc only)"))
	float SlamDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|HitboxDefaults", meta = (DisplayName = "Suggested Radius (doc only)"))
	float SlamRadius = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|HitboxDefaults", meta = (ClampMin = "0", DisplayName = "Suggested Vertical Reach (doc only)"))
	float SlamVerticalReach = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|HitboxDefaults", meta = (DisplayName = "Suggested Hit Stop Frames (doc only)"))
	int32 SlamHitStopFrames = 6;

	/** Queda PÓS-slam (End terminou pinado no ar): velocity Z inicial da soltura.
	 *  0 = gravidade pura — o player cai no Fall Loop da locomoção e pousa com a anim
	 *  de land (AAA). Valores tipo -200 deixam a queda mais seca. NUNCA teleporta. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Feel", meta = (ClampMax = "0"))
	float PostSlamFallVelocity = 0.f;

	/** LEGADO: derruba o alvo no frame 0 sem hitbox — deixe OFF (use ANS_DDRHitbox). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Feel")
	bool bSlamClaimedTargetOnActivate = false;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Feel", meta = (ClampMin = "0", ClampMax = "10", EditCondition = "bSlamClaimedTargetOnActivate"))
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

	/** Quando pular pra secao End da AM_AirSlam. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Sections")
	EDDRSlamEndTrigger SlamEndTrigger = EDDRSlamEndTrigger::BeforeGroundProximity;

	/** BeforeGroundProximity: distancia ao chao (cm) para entrar na secao End ainda no ar. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Sections",
		meta = (ClampMin = "50", EditCondition = "SlamEndTrigger == EDDRSlamEndTrigger::BeforeGroundProximity"))
	float SlamEndGroundProximity = 200.f;

	/** Apos PinInAir no hitbox (so durante Start/Loop): retoma Loop. OFF se End ja comecou. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam|Sections")
	bool bResumeLoopSectionAfterPin = false;

private:
	void CheckSlamEndProximity();
	void TryJumpToEndSection(const TCHAR* Reason);

	FTimerHandle SlamProximityTimerHandle;
	bool bLandedBound = false;
	bool bEndSectionStarted = false;
	// Garante AoE/cue uma unica vez e decide quem encerra a ability (End ou pouso tardio).
	bool bImpactTriggered = false;

	// RootMotionMode do AnimInstance e forcado pra IgnoreRootMotion durante o slam
	// (RM ON num clip de queda "dirigiria" a capsula e mataria a velocity -3500).
	uint8 SavedRootMotionMode = 0;
	bool bSavedRootMotionMode = false;
};
