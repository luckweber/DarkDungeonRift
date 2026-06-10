// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRDashIFrames.h"

#include "DDRGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DDRDashIFrames::UGE_DDRDashIFrames()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.25f);
}

void UGE_DDRDashIFrames::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(DDRTags::State_Movement_Dashing);
	UTargetTagsGameplayEffectComponent& TargetTags = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	TargetTags.SetAndApplyTargetTagChanges(TagContainer);
}
