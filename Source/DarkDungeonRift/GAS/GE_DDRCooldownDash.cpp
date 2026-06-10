// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRCooldownDash.h"

#include "DDRGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DDRCooldownDash::UGE_DDRCooldownDash()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.6f);
}

void UGE_DDRCooldownDash::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	// FindOrAddComponent usa NewObject — não pode rodar no construtor (crash no CDO ao abrir o editor).
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(DDRTags::Cooldown_Dash);
	UTargetTagsGameplayEffectComponent& TargetTags = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	TargetTags.SetAndApplyTargetTagChanges(TagContainer);
}
