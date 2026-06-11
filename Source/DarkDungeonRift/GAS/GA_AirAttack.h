// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AttackLight.h"
#include "GA_AirAttack.generated.h"

// Combo AEREO (doc 16 par.4): herda toda a maquina do combo de chao (secoes, janela,
// buffer) — so muda o gate (requer State.Combat.InAir), as secoes default e o fato de
// cada golpe RENOVAR o hold aereo do player. O re-float do ALVO vem do notify
// ANS_DDRHitbox com bAirPop=true. Mesmo botao do ataque (InputID Attack).
UCLASS()
class DARKDUNGEONRIFT_API UGA_AirAttack : public UGA_AttackLight
{
	GENERATED_BODY()

public:
	UGA_AirAttack();

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void TryAdvanceCombo() override;

	// Soft-lock aéreo: mira o alvo JUGGLEADO (18 §6).
	virtual bool ShouldPreferAirborneTargets() const override { return true; }

	virtual EDDRMotionWarpProfile GetMotionWarpProfile() const override { return EDDRMotionWarpProfile::Air; }

	/** Co-altitude por hit do juggle (ANS_DDRHitbox com bAirPop). */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|AirAttack|Juggle", meta = (ClampMin = "0"))
	float JuggleTargetHeightAbovePlayer = 60.f;

	/** Nudge vertical extra por hit (× decay) — pequeno, não stack infinito. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|AirAttack|Juggle", meta = (ClampMin = "0", ClampMax = "1"))
	float AirPopVerticalNudgeScale = 0.15f;
};
