// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDREnemyTypes.h"
#include "GameFramework/Actor.h"
#include "DDREncounterManager.generated.h"

class ADDREnemyCharacter;
class ADDRProjectileBase;
class UDDREnemyData;

UCLASS()
class DARKDUNGEONRIFT_API ADDREncounterManager : public AActor
{
	GENERATED_BODY()

public:
	ADDREncounterManager();

	UFUNCTION(BlueprintCallable, Category = "DDR|Encounter")
	void StartEncounter();

	UFUNCTION(BlueprintCallable, Category = "DDR|Encounter|Token")
	bool TryGrantAttackToken(ADDREnemyCharacter* Enemy);

	UFUNCTION(BlueprintCallable, Category = "DDR|Encounter|Token")
	void ReleaseAttackToken(ADDREnemyCharacter* Enemy);

	UFUNCTION(BlueprintPure, Category = "DDR|Encounter|Token")
	bool HasAttackToken(const ADDREnemyCharacter* Enemy) const;

	UFUNCTION(BlueprintCallable, Category = "DDR|Encounter")
	void RegisterSpawnPoint(AActor* SpawnPoint);

	static ADDREncounterManager* FindForWorld(const UWorld* World);

protected:
	virtual void BeginPlay() override;

	void StartWave(int32 WaveIndex);
	void SpawnWaveEntry(const FDDRSpawnEntry& Entry, AActor* SpawnPoint);
	void AdvanceWaveIfCleared();
	AActor* PickSpawnPoint() const;
	void NotifyEnemyDied(ADDREnemyCharacter* Enemy);

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Encounter")
	TArray<FDDRWave> Waves;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Encounter|Token", meta = (ClampMin = "1"))
	int32 MaxConcurrentAttackTokens = 2;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Encounter")
	TSubclassOf<ADDREnemyCharacter> DefaultEnemyClass;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Encounter")
	int32 CurrentWaveIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Encounter")
	int32 AliveEnemyCount = 0;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> SpawnPoints;

	UPROPERTY()
	TArray<TWeakObjectPtr<ADDREnemyCharacter>> TokenHolders;

	FTimerHandle WaveDelayHandle;
};
