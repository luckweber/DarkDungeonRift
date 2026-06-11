// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DDRCombatTypes.h"
#include "DDRMotionWarpTypes.h"
#include "DDRCombatComponent.generated.h"

class UAbilitySystemComponent;
class UGE_DDRDamage;

UCLASS(ClassGroup = (DDR), meta = (BlueprintSpawnableComponent))
class DARKDUNGEONRIFT_API UDDRCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDDRCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void PerformMeleeSweep(const FDDRMeleeSweepParams& Params);
	void OpenComboWindow();
	void CloseComboWindow();
	bool IsComboWindowOpen() const { return bComboWindowOpen; }

	void BufferAttackInput(float BufferSeconds);
	void ClearBufferedAttack();
	bool HasBufferedAttack() const;

	void ResetHitTracking();
	void SetActiveComboSection(int32 SectionIndex);

	// ===== Estado aéreo do PLAYER (doc 16 §2: follow launch + hold) =====
	// Trava o dono no ar (Flying, tag State.Combat.InAir) com auto-drop se ficar ocioso.
	void EnterAirCombat();
	// Re-arma o auto-drop (chame a cada air attack).
	void RefreshAirHold();
	// Sai do ar (Falling). bSlam = descida forte (slam).
	void ExitAirCombat(bool bSlam);
	bool IsInAirCombat() const { return bInAirCombat; }
	// Lançou alguém no último sweep com bLaunchTargets? (launcher só sobe se acertou)
	bool DidLaunchTargetThisSwing() const { return bLaunchedTargetThisSwing; }

	/** Ativa arrasto horizontal do alvo juggleado (set 06). Chamado pelo ANS_DDRHitbox. */
	void SetAirCarryActive(bool bActive, float ForwardOffset);
	void SyncJuggleTargetFollow();

	// ===== Soft-lock topdown (doc 18 §6) =====
	// Escolhe o alvo "óbvio": cone na direção da INTENÇÃO (input de movimento > facing),
	// desempate por distância; fallback = mais próximo no raio. bPreferAirborne prioriza
	// o alvo juggleado (combos aéreos).
	UFUNCTION(BlueprintCallable, Category = "DDR|Combat|SoftLock")
	AActor* FindSoftLockTarget(bool bPreferAirborne) const;

	// Vira o dono (yaw) pro alvo do soft-lock no startup do golpe. Retorna o alvo (ou null).
	UFUNCTION(BlueprintCallable, Category = "DDR|Combat|SoftLock")
	AActor* FaceSoftLockTarget(bool bPreferAirborne);

	/** Soft-lock + motion warp (doc 18 §6). Chame no startup de cada golpe, antes da montage. */
	UFUNCTION(BlueprintCallable, Category = "DDR|Combat|MotionWarp")
	AActor* FaceAndSetupMotionWarp(EDDRMotionWarpProfile Profile, bool bPreferAirborne);

	UFUNCTION(BlueprintCallable, Category = "DDR|Combat|MotionWarp")
	void ClearAttackMotionWarp();

	UFUNCTION(BlueprintCallable, Category = "DDR|Combat")
	float GetHealthPercent() const;

	/** Copia tuning do GA_Launcher ao ativar (fallback = valores no Combat Component). */
	void ApplyLauncherAirTuning(float InLaunchRiseHeight, float InJuggleTargetHeightAbovePlayer);

	/** Copia tuning do GA_AirAttack ao ativar (fallback = valores no Combat Component). */
	void ApplyAirAttackJuggleTuning(float InJuggleTargetHeightAbovePlayer, float InAirPopVerticalNudgeScale);

private:
	bool CanHitActor(AActor* OtherActor) const;
	void ApplyDamageToTarget(AActor* TargetActor, const FDDRMeleeSweepParams& Params);
	void SendHitEvent(AActor* HitActor) const;
	UAbilitySystemComponent* GetOwnerASC() const;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat")
	TSubclassOf<UGE_DDRDamage> DamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat")
	float DamageAttackPowerScale = 0.01f;

	// Sockets na STATIC MESH da arma usados pro sweep da lamina (doc 18 par.2).
	// Sem arma ou sem os sockets -> fallback: sweep frontal do ator.
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Weapon")
	FName WeaponTraceSocketStart = TEXT("weapon_start");

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Weapon")
	FName WeaponTraceSocketEnd = TEXT("weapon_end");

	// ===== Juggle (doc 16 §3 — modelo numérico) =====
	/** Fallback global — abilities sobrescrevem ao ativar (GA_Launcher / GA_AirAttack). */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air")
	float LaunchRiseHeight = 300.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air")
	float AirPopHeightBase = 150.f;          // re-float por hit

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float AirPopDecay = 0.85f;               // PopHeight *= decay^hits (anti-infinito)

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air")
	int32 MaxJuggleHits = 7;                 // cap canônico (05 §3.1)

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air")
	float TargetAirborneHoldSeconds = 1.1f;  // hang do ALVO renovado por hit

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air")
	float PlayerAirHoldSeconds = 1.4f;       // auto-drop do PLAYER se ocioso no ar

	/** Fallback global — GA_Launcher (entrada) e GA_AirAttack (pop) sobrescrevem ao ativar. */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air")
	float JuggleTargetHeightAbovePlayer = 60.f;

	/** Fallback global — GA_AirAttack sobrescreve ao ativar. */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|Air", meta = (ClampMin = "0", ClampMax = "1"))
	float AirPopVerticalNudgeScale = 0.15f;

	// ===== Soft-lock (doc 18 §6) =====
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|SoftLock")
	bool bSoftLockEnabled = true;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|SoftLock")
	float SoftLockRadius = 600.f;            // alcance da assistência

	UPROPERTY(EditAnywhere, Category = "DDR|Combat|SoftLock", meta = (ClampMin = "5", ClampMax = "180"))
	float SoftLockHalfAngleDegrees = 75.f;   // cone na direção da intenção

	/** Ignora alvos com |ΔZ| maior que isto — evita lock no inimigo no céu enquanto você está no chão. */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|SoftLock")
	float SoftLockMaxVerticalGap = 220.f;

	// ===== Motion warp (doc 18 §6.3) =====
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|MotionWarp")
	bool bMotionWarpEnabled = true;

	/** Distância ideal do corpo ao alvo após o warp (cm). */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|MotionWarp")
	float IdealHitDistance = 90.f;

	/** Acima disto NÃO warpa — whiff honesto (ground/run). */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|MotionWarp")
	float MaxWarpDistance = 200.f;

	/** Cap menor no ar — alvo já está perto no juggle. */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|MotionWarp")
	float MaxWarpDistanceAir = 120.f;

	/** Cap do launcher — fecha gap horizontal antes do RM vertical do clip. */
	UPROPERTY(EditAnywhere, Category = "DDR|Combat|MotionWarp")
	float MaxWarpDistanceLauncher = 180.f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	bool bComboWindowOpen = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	bool bBufferedAttack = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	float BufferedAttackTimeRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	int32 ActiveComboSection = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat|Air")
	bool bInAirCombat = false;

	bool bLaunchedTargetThisSwing = false;
	uint8 SavedMovementMode = 0;
	FTimerHandle AirHoldTimerHandle;

	/** Altitude do juggle (Z fixo enquanto bInAirCombat — permite RM horizontal nos clips sem cair). */
	float AirAnchorZ = 0.f;
	bool bHasAirAnchor = false;

	void LaunchTarget(AActor* TargetActor);
	void AirPopTarget(AActor* TargetActor);
	void OnPlayerAirHoldExpired();
	void MaintainAirAltitude();

	bool bAirCarryActive = false;
	float AirCarryForwardOffset = 90.f;
	TWeakObjectPtr<AActor> ActiveJuggleTarget;

	TSet<TWeakObjectPtr<AActor>> HitActorsThisSwing;
};
