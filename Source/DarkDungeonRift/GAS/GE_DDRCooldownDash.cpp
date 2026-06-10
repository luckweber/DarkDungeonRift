// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRCooldownDash.h"

#include "DDRGameplayTags.h"

UGE_DDRCooldownDash::UGE_DDRCooldownDash()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.6f);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(DDRTags::Cooldown_Dash);
	InheritableOwnedTagsContainer = TagContainer;
}
