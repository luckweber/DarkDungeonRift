// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_DDRCooldownLauncher.h"

#include "DDRGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DDRCooldownLauncher::UGE_DDRCooldownLauncher()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.8f); // anti-spam do launcher (16 §9)
}

void UGE_DDRCooldownLauncher::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	// FindOrAddComponent usa NewObject — não pode rodar no construtor (crash no CDO ao abrir o editor).
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(DDRTags::Cooldown_Launcher);
	UTargetTagsGameplayEffectComponent& TargetTags = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	TargetTags.SetAndApplyTargetTagChanges(TagContainer);
}
