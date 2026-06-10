// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_Dash.generated.h"

UCLASS()
class DARKDUNGEONRIFT_API UGA_Dash : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dash();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnRootMotionFinished();
	FVector GetDashDirection() const;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	float DashDistance = 550.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	TSubclassOf<UGameplayEffect> IFrameEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;
};
