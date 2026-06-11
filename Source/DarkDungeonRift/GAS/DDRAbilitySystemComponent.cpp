// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRAbilitySystemComponent.h"

#include "DDRAbilityInput.h"
#include "DDRGameplayTags.h"
#include "GA_AirAttack.h"

void UDDRAbilitySystemComponent::AbilityLocalInputPressed(int32 InputID)
{
	// Mesmo botao Attack no juggle: tenta ativar AirAttack antes do Super (que iteraria AttackLight).
	// InputPressed de combo ativo fica com Super — la a engine seta ActorInfo corretamente.
	if (InputID == static_cast<int32>(EDDRAbilityInputID::Attack)
		&& HasMatchingGameplayTag(DDRTags::State_Combat_InAir))
	{
		for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
		{
			if (Spec.InputID != InputID || Spec.IsActive() || !Spec.Ability
				|| !Spec.Ability->IsA(UGA_AirAttack::StaticClass()))
			{
				continue;
			}

			if (TryActivateAbility(Spec.Handle))
			{
				return;
			}
		}
	}

	Super::AbilityLocalInputPressed(InputID);
}
