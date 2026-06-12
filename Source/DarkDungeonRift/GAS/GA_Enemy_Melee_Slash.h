// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_Enemy_Melee_Slash.generated.h"

class UAnimMontage;

UCLASS()
class DARKDUNGEONRIFT_API UGA_Enemy_Melee_Slash : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Enemy_Melee_Slash();

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
	void OnWindupFinished();

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnRecoveryFinished();

	void FaceAggroTarget();
	void PlayTelegraphCue();

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy")
	float WindupSeconds = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy")
	float RecoverySeconds = 0.5f;

	bool bRotationLocked = false;
	bool bSavedOrientToMovement = true;
};
