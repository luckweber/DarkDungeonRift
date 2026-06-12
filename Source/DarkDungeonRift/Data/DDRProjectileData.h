// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "DDRProjectileData.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

UCLASS(BlueprintType)
class DARKDUNGEONRIFT_API UDDRProjectileData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "100"))
	float Speed = 2400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.1"))
	float Lifetime = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	float HitRadius = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float BaseDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float PoiseDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FGameplayTag ImpactCue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<UNiagaraSystem> TracerFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<UNiagaraSystem> ImpactFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> FireSFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> ImpactSFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	FLinearColor TelegraphColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.1"))
	float TelegraphDuration = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	bool bPierce = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	int32 MaxPierceCount = 0;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
