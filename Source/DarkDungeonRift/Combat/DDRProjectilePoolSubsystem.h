// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DDRProjectilePoolSubsystem.generated.h"

class ADDRProjectileBase;

UCLASS()
class DARKDUNGEONRIFT_API UDDRProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	ADDRProjectileBase* Acquire(TSubclassOf<ADDRProjectileBase> ProjectileClass, const FTransform& SpawnTransform);
	void Release(ADDRProjectileBase* Projectile);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Projectile", meta = (ClampMin = "4"))
	int32 InitialPoolSize = 24;

	UPROPERTY()
	TArray<TObjectPtr<ADDRProjectileBase>> InactiveProjectiles;

	UPROPERTY()
	TSubclassOf<ADDRProjectileBase> DefaultProjectileClass;
};
