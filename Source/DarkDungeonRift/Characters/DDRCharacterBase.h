// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "DDRStartupAbilities.h"
#include "GameFramework/Character.h"
#include "DDRCharacterBase.generated.h"

class UDDRAbilitySystemComponent;
class UDDRAttributeSet;
class UDDRCharacterMovementComponent;
class UDDRCombatComponent;
class UMotionWarpingComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class DARKDUNGEONRIFT_API ADDRCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADDRCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "DDR|Movement")
	UDDRCharacterMovementComponent* GetDDRMovement() const;

	UFUNCTION(BlueprintCallable, Category = "DDR|Combat")
	UDDRCombatComponent* GetDDRCombat() const { return CombatComponent; }

	UFUNCTION(BlueprintCallable, Category = "DDR|MotionWarp")
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Weapon")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// ===== Juggle do ALVO (doc 16 §2-3): subir dirigido, segurar, pop, cair. =====
	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void StartAirborne(float RiseHeight, float HoldSeconds);

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void ApplyAirPop(float PopHeight, float HoldSeconds);

	/** Define altura-alvo absoluta do juggle (co-altitude com o atacante). */
	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void SetAirborneTargetZ(float NewTargetZ, float HoldSeconds);

	/** Ajusta Z-alvo sem incrementar hit count (launcher / snap inicial). */
	void OverrideAirborneTargetZ(float NewTargetZ);

	/** Re-arma só o hold (sem pop/hit count) — slam segura o alvo no ar até o impacto. */
	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void ExtendAirborneHold(float HoldSeconds);

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void EndAirborne(bool bSlammed);

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	bool IsAirborne() const { return bAirborneActive; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	int32 GetAirborneHitCount() const { return AirborneHitCount; }

	/** Segue o atacante no plano horizontal enquanto Airborne (juggle que avança, set 06). */
	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void SetAirborneFollow(AActor* Attacker, float ForwardOffset, bool bEnable);

	// ===== Ragdoll knockdown (queda AAA do slam) =====
	/** Vira ragdoll físico (exige Physics Asset na mesh). Retorna false sem PA (fallback cápsula). */
	UFUNCTION(BlueprintCallable, Category = "DDR|Ragdoll")
	bool StartRagdoll(const FVector& InitialVelocity);

	/** Levanta: cápsula vai pro corpo, mesh re-anexa, volta ao fluxo normal. */
	UFUNCTION(BlueprintCallable, Category = "DDR|Ragdoll")
	void RecoverFromRagdoll();

	UFUNCTION(BlueprintCallable, Category = "DDR|Ragdoll")
	bool IsRagdolled() const { return bRagdolled; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	void InitializeAbilitySystem();
	void GrantStartupAbilities();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDDRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDDRCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DDR|MotionWarp")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	// Arma visual (M1: espada fixa). A mesh NUNCA colide — o hit é o sweep do CombatComponent.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DDR|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// Socket do skeleton onde a arma anexa (crie no SK_Mannequin: bone hand_r -> Add Socket).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Weapon")
	FName WeaponSocketName = TEXT("weapon_r");

	UPROPERTY()
	TObjectPtr<UDDRAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Abilities")
	TArray<FDDRCStartupAbility> StartupAbilities;

	// Velocidade de interp da subida/pop do juggle (uu/s de aproximação).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Airborne")
	float AirborneInterpSpeed = 10.f;

	// Queda forçada pelo slam (negativa = pra baixo). MAIS rápida que a descida do
	// player (-3500): o alvo arremessado no apex esmaga no chão PRIMEIRO (coreografia).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Airborne")
	float SlammedFallVelocity = -4500.f;

	// ===== Ragdoll (knockdown do slam) =====
	/** Slam derruba com RAGDOLL (física real, AAA) em vez de queda de cápsula com pose
	 *  congelada ("teleporte"). Exige Physics Asset na skeletal mesh (SK_Mannequin tem). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ragdoll")
	bool bRagdollOnSlammed = true;

	/** Tempo caído até levantar (0 = nunca — fica no chão). Inimigos M3: trocar por anim getup (P1). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ragdoll", meta = (ClampMin = "0"))
	float RagdollRecoverSeconds = 1.5f;

	/** Osso que a cápsula segue durante o ragdoll (sombra blob/soft-lock acompanham o corpo). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ragdoll")
	FName RagdollFollowBone = TEXT("pelvis");

	/** Cap da velocity inicial dos corpos (cm/s). O -4500 do slam TUNELAVA o chão fino;
	 *  física não precisa de velocidade pra parecer pesada. CCD também é ligado na queda. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ragdoll", meta = (ClampMin = "100"))
	float RagdollMaxInitialSpeed = 1200.f;

	/** Pelvis não desce abaixo disto do impacto no chão (evita tunelar durante ragdoll). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ragdoll", meta = (ClampMin = "10"))
	float RagdollPelvisGroundClearance = 55.f;

	/** Velocity Z aplicada ao ragdoll no pouso do slam (corpo ja no chao — impacto visual). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Ragdoll", meta = (ClampMin = "50"))
	float SlamRagdollImpactSpeed = 350.f;

	bool bAbilitySystemInitialized = false;

private:
	float GetSlamFallSpeed() const;
	bool TraceFloorBelow(const FVector& QueryLoc, FHitResult& OutHit) const;
	bool SweepCapsuleToFloor(const FVector& From, const FVector& To, FHitResult& OutHit) const;
	void BeginSlamKnockdown(float SlamFallSpeed);
	void FinishGuidedSlamFall();
	void TickGuidedSlamFall(float DeltaSeconds);
	void TickRagdollFollow();

	void RestartAirborneHoldTimer(float HoldSeconds);
	void OnAirborneHoldExpired();
	void OnRagdollRecoverExpired();

	bool bAirborneActive = false;
	float AirborneTargetZ = 0.f;
	int32 AirborneHitCount = 0;
	FTimerHandle AirborneHoldTimerHandle;

	bool bAirborneFollowEnabled = false;
	float AirborneFollowForwardOffset = 90.f;
	TWeakObjectPtr<AActor> AirborneFollowAttacker;

	bool bRagdolled = false;
	bool bGuidedSlamFall = false;
	bool bPendingRagdollOnSlamLand = false;
	float GuidedSlamFallStartTime = 0.f;
	static constexpr float GuidedSlamFallTimeoutSeconds = 3.f;
	FTimerHandle RagdollRecoverTimerHandle;
	// Capturados no BeginPlay pra restaurar EXATAMENTE no recover (BP pode ter ajustado).
	FTransform DefaultMeshRelativeTransform;
	FName DefaultMeshCollisionProfile = TEXT("CharacterMesh");
	// Fallback do recover se o corpo tunelar pro void (sem chão no trace).
	FVector RagdollStartActorLocation = FVector::ZeroVector;
};
