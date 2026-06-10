// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRDamage.h"

#include "DDRAttributeSet.h"
#include "DDRGameplayTags.h"

UGE_DDRDamage::UGE_DDRDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UDDRAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DDRTags::Data_Damage;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}
