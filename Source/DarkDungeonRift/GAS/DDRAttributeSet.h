// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "DDRAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class DARKDUNGEONRIFT_API UDDRAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UDDRAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UDDRAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UDDRAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UDDRAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UDDRAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UDDRAttributeSet, AttackPower)

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UDDRAttributeSet, IncomingDamage)

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
