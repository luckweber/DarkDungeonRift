// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRGameplayTags.h"

namespace DDRTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Light, "Ability.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dash, "Ability.Dash");

	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Attacking, "State.Combat.Attacking");
	UE_DEFINE_GAMEPLAY_TAG(State_Movement_Sprinting, "State.Movement.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(State_Movement_Dashing, "State.Movement.Dashing");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dash, "Cooldown.Dash");

	UE_DEFINE_GAMEPLAY_TAG(Faction_Enemy, "Faction.Enemy");

	UE_DEFINE_GAMEPLAY_TAG(Event_Combat_Hit, "Event.Combat.Hit");

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");

	UE_DEFINE_GAMEPLAY_TAG(Cue_Hit_Light, "GameplayCue.Hit.Light");
}
