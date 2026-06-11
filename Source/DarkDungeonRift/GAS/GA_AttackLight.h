// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "DDRMotionWarpTypes.h"
#include "GA_AttackLight.generated.h"

class UAnimMontage;

UCLASS()
class DARKDUNGEONRIFT_API UGA_AttackLight : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AttackLight();

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

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);
	virtual void TryAdvanceCombo();
	virtual void PlayCurrentSection();
	void PlayRunAttackOpener();

	// Soft-lock (doc 18 §6): combos aéreos priorizam o alvo juggleado.
	virtual bool ShouldPreferAirborneTargets() const { return false; }

	// Motion warp por verbo (doc 18 §6.3).
	virtual EDDRMotionWarpProfile GetMotionWarpProfile() const { return EDDRMotionWarpProfile::Ground; }

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack")
	TObjectPtr<UAnimMontage> ComboMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack")
	TArray<FName> ComboSections = { TEXT("Atk1"), TEXT("Atk2"), TEXT("Atk3"), TEXT("Atk4") };

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack", meta = (ClampMin = "0.05"))
	float InputBufferSeconds = 0.3f;

	// OPENER em corrida (Run_Attack_01): se Speed >= RunAttackMinSpeed ao INICIAR o combo,
	// toca esta montage primeiro; a janela dela encadeia no Atk1 do combo normal.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack|RunAttack")
	TObjectPtr<UAnimMontage> RunAttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack|RunAttack")
	float RunAttackMinSpeed = 450.f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Attack")
	int32 ComboIndex = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Attack")
	bool bPlayingRunAttack = false;

	// Troca de montage (opener->combo) interrompe a task anterior — ignorar esse interrupt 1x.
	bool bIgnoreNextInterrupt = false;

	// Trava de rotação durante o combo (o facing do soft-lock manda; WASD não gira o corpo).
	bool bSavedOrientToMovement = false;
	bool bRotationLocked = false;
};
