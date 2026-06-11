// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_AirSlam.generated.h"

class UAnimMontage;

// Finisher descendente (doc 16 par.5): desce rapido, no IMPACTO pula pra secao "End",
// AoE de dano + derruba os Airborne + hit-stop pesado. Montage com secoes "Start"/"End"
// (Attack_Air_To_Floor_Start / _End).
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

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	FName LandSection = TEXT("End");

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	float SlamDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	float SlamRadius = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Slam")
	int32 SlamHitStopFrames = 6;

private:
	bool bLandedBound = false;
};
