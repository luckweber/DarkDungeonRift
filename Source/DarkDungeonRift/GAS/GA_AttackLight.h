// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_AttackLight.generated.h"

class UAnimMontage;

UCLASS()
class DARKDUNGEONRIFT_API UGA_AttackLight : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AttackLight();

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
	void TryAdvanceCombo();
	void PlayCurrentSection();

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack")
	TObjectPtr<UAnimMontage> ComboMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack")
	TArray<FName> ComboSections = { TEXT("Atk1"), TEXT("Atk2"), TEXT("Atk3"), TEXT("Atk4") };

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Attack", meta = (ClampMin = "0.05"))
	float InputBufferSeconds = 0.3f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Attack")
	int32 ComboIndex = 0;
};
