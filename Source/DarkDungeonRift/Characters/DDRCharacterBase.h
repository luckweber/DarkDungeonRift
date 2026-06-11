// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "DDRStartupAbilities.h"
#include "GameFramework/Character.h"
#include "DDRCharacterBase.generated.h"

class UDDRAbilitySystemComponent;
class UDDRAttributeSet;
class UDDRCharacterMovementComponent;
class UDDRCombatComponent;
class UMotionWarpingComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class DARKDUNGEONRIFT_API ADDRCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADDRCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "DDR|Movement")
	UDDRCharacterMovementComponent* GetDDRMovement() const;

	UFUNCTION(BlueprintCallable, Category = "DDR|Combat")
	UDDRCombatComponent* GetDDRCombat() const { return CombatComponent; }

	UFUNCTION(BlueprintCallable, Category = "DDR|MotionWarp")
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Weapon")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// ===== Juggle do ALVO (doc 16 §2-3): subir dirigido, segurar, pop, cair. =====
	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void StartAirborne(float RiseHeight, float HoldSeconds);

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void ApplyAirPop(float PopHeight, float HoldSeconds);

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	void EndAirborne(bool bSlammed);

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	bool IsAirborne() const { return bAirborneActive; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Airborne")
	int32 GetAirborneHitCount() const { return AirborneHitCount; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	void InitializeAbilitySystem();
	void GrantStartupAbilities();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDDRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDDRCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DDR|MotionWarp")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	// Arma visual (M1: espada fixa). A mesh NUNCA colide — o hit é o sweep do CombatComponent.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DDR|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// Socket do skeleton onde a arma anexa (crie no SK_Mannequin: bone hand_r -> Add Socket).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Weapon")
	FName WeaponSocketName = TEXT("weapon_r");

	UPROPERTY()
	TObjectPtr<UDDRAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Abilities")
	TArray<FDDRCStartupAbility> StartupAbilities;

	// Velocidade de interp da subida/pop do juggle (uu/s de aproximação).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Airborne")
	float AirborneInterpSpeed = 10.f;

	// Queda forçada pelo slam (negativa = pra baixo, rápida).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Airborne")
	float SlammedFallVelocity = -3000.f;

	bool bAbilitySystemInitialized = false;

private:
	void RestartAirborneHoldTimer(float HoldSeconds);
	void OnAirborneHoldExpired();

	bool bAirborneActive = false;
	float AirborneTargetZ = 0.f;
	int32 AirborneHitCount = 0;
	FTimerHandle AirborneHoldTimerHandle;
};
