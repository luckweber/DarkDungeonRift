// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DDRCombatTypes.h"
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

	UFUNCTION(BlueprintCallable, Category = "DDR|Combat")
	float GetHealthPercent() const;

private:
	bool CanHitActor(AActor* OtherActor) const;
	void ApplyDamageToTarget(AActor* TargetActor, const FDDRMeleeSweepParams& Params);
	void SendHitEvent(AActor* HitActor) const;
	UAbilitySystemComponent* GetOwnerASC() const;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat")
	TSubclassOf<UGE_DDRDamage> DamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat")
	float DamageAttackPowerScale = 0.01f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	bool bComboWindowOpen = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	bool bBufferedAttack = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	float BufferedAttackTimeRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Combat")
	int32 ActiveComboSection = 0;

	TSet<TWeakObjectPtr<AActor>> HitActorsThisSwing;
};
