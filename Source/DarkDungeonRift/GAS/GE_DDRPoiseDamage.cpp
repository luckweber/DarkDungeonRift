// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRPoiseDamage.h"

#include "DDRAttributeSet.h"
#include "DDRGameplayTags.h"

UGE_DDRPoiseDamage::UGE_DDRPoiseDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UDDRAttributeSet::GetIncomingPoiseDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DDRTags::Data_PoiseDamage;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}
