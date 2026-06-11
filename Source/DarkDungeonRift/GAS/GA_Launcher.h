// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_Launcher.generated.h"

class UAnimMontage;

// Inicia o aereo (doc 16 par.2): uppercut que LANCA o alvo (notify ANS_DDRHitbox com
// bLaunchTargets) e sobe o player junto via ROOT MOTION do clip (Attack_Up_Floor_To_Air).
// Follow launch so acontece se acertou alguem (whiff = termina no chao).
UCLASS()
class DARKDUNGEONRIFT_API UGA_Launcher : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Launcher();

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
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	// Attack_Up_Floor_To_Air (root motion sobe o player) — sem secoes.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Launcher")
	TObjectPtr<UAnimMontage> LauncherMontage;

	/** Altura inicial do alvo no hit (cm). Tune para combinar com o ΔZ do RM do clip. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Launcher|Air", meta = (ClampMin = "0"))
	float LaunchRiseHeight = 300.f;

	/** Co-altitude ao entrar no ar (fim da montage): alvo fica este valor acima do player. */
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Launcher|Air", meta = (ClampMin = "0"))
	float JuggleTargetHeightAbovePlayer = 60.f;

private:
	// MOVE_Walking DESCARTA o Z do root motion (gruda no chao) — o launcher precisa de
	// Flying durante a montage pro clip Floor_To_Air subir o player de verdade.
	bool bSetFlyingForLaunch = false;
};
