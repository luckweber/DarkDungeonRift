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

	UFUNCTION(BlueprintCallable, Category = "DDR|Weapon")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

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

	bool bAbilitySystemInitialized = false;
};
