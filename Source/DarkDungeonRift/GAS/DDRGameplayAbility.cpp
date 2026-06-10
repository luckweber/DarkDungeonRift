// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRGameplayAbility.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"

UDDRGameplayAbility::UDDRGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(DDRTags::State_Dead);
	ActivationBlockedTags = BlockedTags;
}

UDDRAbilitySystemComponent* UDDRGameplayAbility::GetDDRAbilitySystemComponent() const
{
	return Cast<UDDRAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

UDDRCombatComponent* UDDRGameplayAbility::GetDDRCombatComponent() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar ? Avatar->FindComponentByClass<UDDRCombatComponent>() : nullptr;
}
