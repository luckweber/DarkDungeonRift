// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AirSlam.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DDRCombatComponent.h"
#include "DDRCombatTypes.h"
#include "DDRMotionWarpTypes.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GameFramework/Character.h"

UGA_AirSlam::UGA_AirSlam()
{
	AbilityTags.AddTag(DDRTags::Ability_Attack);
	AbilityTags.AddTag(DDRTags::Ability_Attack_AirSlam);

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(DDRTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;

	// So no ar; fecha o juggle cortando o air attack.
	ActivationRequiredTags.AddTag(DDRTags::State_Combat_InAir);

	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(DDRTags::Ability_Attack_Air);
	CancelAbilitiesWithTag = CancelTags;
}

void UGA_AirSlam::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SlamMontage)
	{
		UE_LOG(LogDDR, Warning, TEXT("GA_AirSlam: SlamMontage not assigned on %s"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Pouso dispara o impacto.
	Character->LandedDelegate.AddDynamic(this, &UGA_AirSlam::OnSlamLanded);
	bLandedBound = true;

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SlamMontage, 1.f, StartSection, /*bStopWhenAbilityEnds=*/false);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_AirSlam::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_AirSlam::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_AirSlam::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_AirSlam::OnMontageFinished);
	MontageTask->ReadyForActivation();

	// Sai do hold aereo e DESCE com forca (16 par.5).
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->FaceAndSetupMotionWarp(EDDRMotionWarpProfile::Slam, /*bPreferAirborne=*/true);
		Combat->ExitAirCombat(/*bSlam=*/true);
	}
}

void UGA_AirSlam::OnSlamLanded(const FHitResult& Hit)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	// Anim: pula pra fase de impacto.
	if (Character)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(SlamMontage)
					&& SlamMontage->GetSectionIndex(LandSection) != INDEX_NONE)
				{
					AnimInstance->Montage_JumpToSection(LandSection, SlamMontage);
				}
			}
		}
	}

	// AoE no ponto de impacto: dano + derruba os Airborne + hit-stop pesado.
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		FDDRMeleeSweepParams AoE;
		AoE.BaseDamage = SlamDamage;
		AoE.SweepRadius = SlamRadius;
		AoE.HitStopFrames = SlamHitStopFrames;
		AoE.bAoEAtOwner = true;
		AoE.bSlamDownTargets = true;

		Combat->ResetHitTracking();
		Combat->PerformMeleeSweep(AoE);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = Character;
		CueParams.Location = Character ? Character->GetActorLocation() : FVector::ZeroVector;
		ASC->ExecuteGameplayCue(DDRTags::Cue_Slam, CueParams);
	}
}

void UGA_AirSlam::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_AirSlam::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bLandedBound)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			Character->LandedDelegate.RemoveDynamic(this, &UGA_AirSlam::OnSlamLanded);
		}
		bLandedBound = false;
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ClearAttackMotionWarp();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
