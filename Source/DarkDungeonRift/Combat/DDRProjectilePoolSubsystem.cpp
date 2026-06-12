// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRProjectilePoolSubsystem.h"

#include "DDRProjectileBase.h"

ADDRProjectileBase* UDDRProjectilePoolSubsystem::Acquire(
	TSubclassOf<ADDRProjectileBase> ProjectileClass,
	const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!ProjectileClass)
	{
		ProjectileClass = DefaultProjectileClass;
	}
	if (!ProjectileClass)
	{
		ProjectileClass = ADDRProjectileBase::StaticClass();
	}

	while (InactiveProjectiles.Num() > 0)
	{
		ADDRProjectileBase* Candidate = InactiveProjectiles.Pop();
		if (IsValid(Candidate))
		{
			Candidate->SetActorTransform(SpawnTransform);
			return Candidate;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ADDRProjectileBase>(ProjectileClass, SpawnTransform, Params);
}

void UDDRProjectilePoolSubsystem::Release(ADDRProjectileBase* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	InactiveProjectiles.Add(Projectile);
}
