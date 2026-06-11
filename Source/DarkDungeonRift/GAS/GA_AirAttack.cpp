// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AirAttack.h"

#include "AbilitySystemComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"

UGA_AirAttack::UGA_AirAttack()
{
	// Identidade: Ability.Attack.Air (troca a .Light herdada).
	AbilityTags.RemoveTag(DDRTags::Ability_Attack_Light);
	AbilityTags.AddTag(DDRTags::Ability_Attack_Air);

	// So no ar — e NAO bloqueia por InAir (inverte o gate do pai).
	ActivationBlockedTags.RemoveTag(DDRTags::State_Combat_InAir);
	ActivationRequiredTags.AddTag(DDRTags::State_Combat_InAir);

	// Nao ativa no meio do launcher/outro golpe — espera a montage terminar.
	ActivationBlockedTags.AddTag(DDRTags::State_Combat_Attacking);

	// Secoes do combo aereo (Air1 = Attack_Up_Air_To_Air; Air2 = Combo_Attack_Air...).
	ComboSections = { TEXT("Air1"), TEXT("Air2"), TEXT("Air3"), TEXT("Air4") };

	// Sem opener de corrida no ar.
	RunAttackMontage = nullptr;
}

bool UGA_AirAttack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Nao chame GA_AttackLight::CanActivate — ele bloqueia InAir.
	if (!UDDRGameplayAbility::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UDDRCombatComponent* Combat = Avatar ? Avatar->FindComponentByClass<UDDRCombatComponent>() : nullptr;

	const bool bHasInAirTag = ASC && ASC->HasMatchingGameplayTag(DDRTags::State_Combat_InAir);
	const bool bInAirCombat = Combat && Combat->IsInAirCombat();
	if (!bHasInAirTag && !bInAirCombat)
	{
		return false;
	}

	if (!ComboMontage)
	{
		return false;
	}

	return true;
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
			Combat->ApplyAirAttackJuggleTuning(JuggleTargetHeightAbovePlayer, AirPopVerticalNudgeScale);
			Combat->RefreshAirHold();
			UE_LOG(LogDDR, Log, TEXT("[ATK-AIR] juggle tuning (coAlt=%.0f nudge=%.2f) + hold do player renovado"),
				JuggleTargetHeightAbovePlayer, AirPopVerticalNudgeScale);
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
