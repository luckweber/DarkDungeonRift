// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_Enemy_Ranged_Bolt.generated.h"

class ADDRProjectileBase;
class UDDRProjectileData;

UCLASS()
class DARKDUNGEONRIFT_API UGA_Enemy_Ranged_Bolt : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Enemy_Ranged_Bolt();

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
	void OnRecoveryFinished();

	AActor* GetAggroTarget() const;
	void SpawnBolt();
	void PlayTelegraphCue();

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy|Ranged")
	TSubclassOf<ADDRProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy|Ranged")
	TObjectPtr<UDDRProjectileData> ProjectileData;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy|Ranged")
	FName MuzzleSocketName = TEXT("muzzle");

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Enemy|Ranged")
	float RecoverySeconds = 0.6f;

	bool bRotationLocked = false;
	bool bSavedOrientToMovement = true;
};
