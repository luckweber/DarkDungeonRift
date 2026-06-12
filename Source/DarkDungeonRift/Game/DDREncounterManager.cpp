// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDREncounterManager.h"

#include "DDREnemyCharacter.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "AbilitySystemComponent.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"

ADDREncounterManager::ADDREncounterManager()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultEnemyClass = ADDREnemyCharacter::StaticClass();
}

void ADDREncounterManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(FName(TEXT("EnemySpawn"))))
		{
			RegisterSpawnPoint(*It);
		}
	}

	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogDDR, Warning, TEXT("[ENCOUNTER] %s sem TargetPoints tag EnemySpawn — adicione na arena."), *GetName());
	}
}

ADDREncounterManager* ADDREncounterManager::FindForWorld(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ADDREncounterManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

void ADDREncounterManager::RegisterSpawnPoint(AActor* SpawnPoint)
{
	if (SpawnPoint)
	{
		SpawnPoints.AddUnique(SpawnPoint);
	}
}

void ADDREncounterManager::StartEncounter()
{
	if (Waves.Num() == 0)
	{
		UE_LOG(LogDDR, Warning, TEXT("[ENCOUNTER] %s sem ondas configuradas."), *GetName());
		return;
	}

	CurrentWaveIndex = INDEX_NONE;
	AliveEnemyCount = 0;
	TokenHolders.Reset();
	StartWave(0);
}

void ADDREncounterManager::StartWave(const int32 WaveIndex)
{
	if (!Waves.IsValidIndex(WaveIndex))
	{
		UE_LOG(LogDDR, Log, TEXT("[ENCOUNTER] ondas completas t=%.2f"), GetWorld()->GetTimeSeconds());
		return;
	}

	CurrentWaveIndex = WaveIndex;
	const FDDRWave& Wave = Waves[WaveIndex];

	auto SpawnWave = [this, Wave]()
	{
		int32 Spawned = 0;
		for (const FDDRSpawnEntry& Entry : Wave.Spawns)
		{
			for (int32 i = 0; i < Entry.Count; ++i)
			{
				if (Spawned >= Wave.MaxAlive)
				{
					break;
				}

				SpawnWaveEntry(Entry, PickSpawnPoint());
				++Spawned;
			}
		}

		UE_LOG(LogDDR, Log, TEXT("[ENCOUNTER] onda %d iniciada (%d spawns)"), CurrentWaveIndex, Spawned);
	};

	if (Wave.DelayBeforeSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(WaveDelayHandle, SpawnWave, Wave.DelayBeforeSeconds, false);
	}
	else
	{
		SpawnWave();
	}
}

AActor* ADDREncounterManager::PickSpawnPoint() const
{
	TArray<AActor*> Valid;
	for (const TWeakObjectPtr<AActor>& Point : SpawnPoints)
	{
		if (Point.IsValid())
		{
			Valid.Add(Point.Get());
		}
	}

	if (Valid.Num() == 0)
	{
		return const_cast<ADDREncounterManager*>(this);
	}

	return Valid[FMath::RandRange(0, Valid.Num() - 1)];
}

void ADDREncounterManager::SpawnWaveEntry(const FDDRSpawnEntry& Entry, AActor* SpawnPoint)
{
	if (!Entry.EnemyData)
	{
		return;
	}

	const FVector SpawnLoc = SpawnPoint ? SpawnPoint->GetActorLocation() : GetActorLocation();
	const FRotator SpawnRot = SpawnPoint ? SpawnPoint->GetActorRotation() : GetActorRotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ADDREnemyCharacter* Enemy = GetWorld()->SpawnActor<ADDREnemyCharacter>(
		DefaultEnemyClass,
		SpawnLoc,
		SpawnRot,
		Params);

	if (!Enemy)
	{
		return;
	}

	Enemy->ApplyEnemyData(Entry.EnemyData);
	++AliveEnemyCount;

	if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
	{
		TWeakObjectPtr<ADDREnemyCharacter> WeakEnemy = Enemy;
		ASC->RegisterGameplayTagEvent(DDRTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddLambda([this, WeakEnemy](const FGameplayTag Tag, int32 NewCount)
			{
				if (NewCount > 0 && WeakEnemy.IsValid())
				{
					NotifyEnemyDied(WeakEnemy.Get());
				}
			});
	}
}

void ADDREncounterManager::NotifyEnemyDied(ADDREnemyCharacter* Enemy)
{
	ReleaseAttackToken(Enemy);
	--AliveEnemyCount;
	TokenHolders.RemoveAll([](const TWeakObjectPtr<ADDREnemyCharacter>& Holder)
	{
		return !Holder.IsValid();
	});
	AdvanceWaveIfCleared();
}

void ADDREncounterManager::AdvanceWaveIfCleared()
{
	if (AliveEnemyCount > 0)
	{
		return;
	}

	StartWave(CurrentWaveIndex + 1);
}

bool ADDREncounterManager::TryGrantAttackToken(ADDREnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return false;
	}

	TokenHolders.RemoveAll([](const TWeakObjectPtr<ADDREnemyCharacter>& Holder)
	{
		return !Holder.IsValid();
	});

	if (TokenHolders.Contains(Enemy))
	{
		return true;
	}

	if (TokenHolders.Num() >= MaxConcurrentAttackTokens)
	{
		return false;
	}

	TokenHolders.Add(Enemy);
	return true;
}

void ADDREncounterManager::ReleaseAttackToken(ADDREnemyCharacter* Enemy)
{
	TokenHolders.Remove(Enemy);
}

bool ADDREncounterManager::HasAttackToken(const ADDREnemyCharacter* Enemy) const
{
	return Enemy && TokenHolders.Contains(const_cast<ADDREnemyCharacter*>(Enemy));
}
