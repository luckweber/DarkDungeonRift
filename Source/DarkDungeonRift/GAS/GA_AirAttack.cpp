// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AirAttack.h"

#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"

UGA_AirAttack::UGA_AirAttack()
{
	// Identidade: Ability.Attack.Air (troca a .Light herdada).
	AbilityTags.RemoveTag(DDRTags::Ability_Attack_Light);
	AbilityTags.AddTag(DDRTags::Ability_Attack_Air);

	// So no ar — e NAO bloqueia por InAir (inverte o gate do pai).
	ActivationBlockedTags.RemoveTag(DDRTags::State_Combat_InAir);
	ActivationRequiredTags.AddTag(DDRTags::State_Combat_InAir);

	// Secoes do combo aereo (Air1 = Attack_Up_Air_To_Air; Air2 = Combo_Attack_Air...).
	ComboSections = { TEXT("Air1"), TEXT("Air2") };

	// Sem opener de corrida no ar.
	RunAttackMontage = nullptr;
}

void UGA_AirAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Cada ataque aereo renova o hold do player (anti auto-drop, doc 16 par.2).
	if (IsActive())
	{
		if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
		{
			Combat->RefreshAirHold();
		}
	}
}

void UGA_AirAttack::TryAdvanceCombo()
{
	Super::TryAdvanceCombo();

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->RefreshAirHold();
	}
}
