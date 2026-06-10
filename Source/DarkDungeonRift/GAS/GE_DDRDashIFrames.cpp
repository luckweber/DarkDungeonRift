// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRDashIFrames.h"

#include "DDRGameplayTags.h"

UGE_DDRDashIFrames::UGE_DDRDashIFrames()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.25f);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(DDRTags::State_Movement_Dashing);
	InheritableOwnedTagsContainer = TagContainer;
}
