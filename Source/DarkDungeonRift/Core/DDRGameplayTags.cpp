// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRGameplayTags.h"

#include "DDREnemyTypes.h"

namespace DDRTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Light, "Ability.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Launcher, "Ability.Attack.Launcher");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Air, "Ability.Attack.Air");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_AirSlam, "Ability.Attack.AirSlam");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dash, "Ability.Dash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Melee_Slash, "Ability.Enemy.Melee.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Ranged_Bolt, "Ability.Enemy.Ranged.Bolt");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Elite_Smash, "Ability.Enemy.Elite.Smash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Elite_Shove, "Ability.Enemy.Elite.Shove");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss_Charge, "Ability.Boss.Charge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss_AoE, "Ability.Boss.AoE");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss_Combo, "Ability.Boss.Combo");

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Grunt_Melee, "Enemy.Grunt.Melee");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ranged_Archer, "Enemy.Ranged.Archer");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Elite_Brute, "Enemy.Elite.Brute");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Boss_Guardian, "Enemy.Boss.Guardian");

	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Attacking, "State.Combat.Attacking");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Stagger, "State.Combat.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_InAir, "State.Combat.InAir");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Airborne, "State.Combat.Airborne");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_SlamFall, "State.Combat.SlamFall");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Boss_Vulnerable, "State.Combat.Boss.Vulnerable");
	UE_DEFINE_GAMEPLAY_TAG(State_Movement_Sprinting, "State.Movement.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(State_Movement_Dashing, "State.Movement.Dashing");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dash, "Cooldown.Dash");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Launcher, "Cooldown.Launcher");

	UE_DEFINE_GAMEPLAY_TAG(Faction_Enemy, "Faction.Enemy");
	UE_DEFINE_GAMEPLAY_TAG(Faction_Boss, "Faction.Boss");
	UE_DEFINE_GAMEPLAY_TAG(Faction_Boss_NoLaunch, "Faction.Boss.NoLaunch");

	UE_DEFINE_GAMEPLAY_TAG(Event_Combat_Hit, "Event.Combat.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Combat_Launch, "Event.Combat.Launch");
	UE_DEFINE_GAMEPLAY_TAG(Event_Combat_SlamLand, "Event.Combat.SlamLand");
	UE_DEFINE_GAMEPLAY_TAG(Event_Combat_PoiseBreak, "Event.Combat.PoiseBreak");

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_PoiseDamage, "Data.PoiseDamage");

	UE_DEFINE_GAMEPLAY_TAG(Cue_Hit_Light, "GameplayCue.Hit.Light");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Enemy_Telegraph, "GameplayCue.Enemy.Telegraph");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Proj_Telegraph, "GameplayCue.Proj.Telegraph");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Proj_Impact, "GameplayCue.Proj.Impact");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Launch, "GameplayCue.Launch");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Slam, "GameplayCue.Slam");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Death_Enemy, "GameplayCue.Death.Enemy");
}

FGameplayTag DDRTags::GetDefaultEnemyIdForRole(const EDDREnemyRole Role)
{
	switch (Role)
	{
	case EDDREnemyRole::Ranged:  return Enemy_Ranged_Archer;
	case EDDREnemyRole::Elite:   return Enemy_Elite_Brute;
	case EDDREnemyRole::Boss:    return Enemy_Boss_Guardian;
	case EDDREnemyRole::Grunt:
	default:                     return Enemy_Grunt_Melee;
	}
}
