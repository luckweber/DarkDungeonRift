// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DDRProjectileBase.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UDDRProjectileData;

UCLASS()
class DARKDUNGEONRIFT_API ADDRProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	ADDRProjectileBase();

	void InitProjectile(UDDRProjectileData* InData, AActor* InInstigator, const FVector& LaunchDirection);
	void DeactivateProjectile();
	bool IsProjectileActive() const { return bActive; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ApplyHit(AActor* HitActor);
	void ExpireProjectile();

	UPROPERTY(VisibleAnywhere, Category = "DDR|Projectile")
	TObjectPtr<USphereComponent> Sphere = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DDR|Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement = nullptr;

	UPROPERTY()
	TObjectPtr<UDDRProjectileData> ProjectileData = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	int32 PierceCount = 0;
	bool bActive = false;

	FTimerHandle LifetimeTimerHandle;
};
