// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "DDRCharacterBase.generated.h"

class UDDRAbilitySystemComponent;
class UDDRAttributeSet;
class UDDRCharacterMovementComponent;

UCLASS(Abstract)
class DARKDUNGEONRIFT_API ADDRCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADDRCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "DDR|Movement")
	UDDRCharacterMovementComponent* GetDDRMovement() const;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	void InitializeAbilitySystem();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDDRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UDDRAttributeSet> AttributeSet;
};
