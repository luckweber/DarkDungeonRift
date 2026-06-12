// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Enemy_Melee_Slash.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "DDREnemyCharacter.h"
#include "DDREnemyData.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Enemy_Melee_Slash::UGA_Enemy_Melee_Slash()
{
	AbilityTags.AddTag(DDRTags::Ability_Enemy_Melee_Slash);

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(DDRTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;

	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(DDRTags::State_Combat_Airborne);
	BlockedTags.AddTag(DDRTags::State_Combat_Stagger);
	ActivationBlockedTags.AppendTags(BlockedTags);
}

void UGA_Enemy_Melee_Slash::ActivateAbility(
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

	if (const ADDREnemyCharacter* Enemy = Cast<ADDREnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (const UDDREnemyData* Data = Enemy->GetEnemyData())
		{
			WindupSeconds = Data->Telegraph.WindupSeconds;
		}
	}

	FaceAggroTarget();

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			bSavedOrientToMovement = MoveComp->bOrientRotationToMovement;
			MoveComp->bOrientRotationToMovement = false;
			bRotationLocked = true;
		}
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ResetHitTracking();
	}

	PlayTelegraphCue();

	UAbilityTask_WaitDelay* WindupTask = UAbilityTask_WaitDelay::WaitDelay(this, WindupSeconds);
	WindupTask->OnFinish.AddDynamic(this, &UGA_Enemy_Melee_Slash::OnWindupFinished);
	WindupTask->ReadyForActivation();
}

void UGA_Enemy_Melee_Slash::OnWindupFinished()
{
	if (!AttackMontage)
	{
		UE_LOG(LogDDR, Warning, TEXT("GA_Enemy_Melee_Slash: AttackMontage not assigned."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Enemy_Melee_Slash::OnMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Enemy_Melee_Slash::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Enemy_Melee_Slash::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Enemy_Melee_Slash::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Enemy_Melee_Slash::OnMontageEnded()
{
	if (RecoverySeconds > KINDA_SMALL_NUMBER)
	{
		UAbilityTask_WaitDelay* RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, RecoverySeconds);
		RecoveryTask->OnFinish.AddDynamic(this, &UGA_Enemy_Melee_Slash::OnRecoveryFinished);
		RecoveryTask->ReadyForActivation();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Enemy_Melee_Slash::OnRecoveryFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Enemy_Melee_Slash::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Enemy_Melee_Slash::FaceAggroTarget()
{
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->FaceSoftLockTarget(/*bPreferAirborne=*/false);
	}
}

void UGA_Enemy_Melee_Slash::PlayTelegraphCue()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetAvatarActorFromActorInfo();
		CueParams.Location = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorLocation() : FVector::ZeroVector;
		ASC->ExecuteGameplayCue(DDRTags::Cue_Enemy_Telegraph, CueParams);
	}
}

void UGA_Enemy_Melee_Slash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (bRotationLocked)
	{
		bRotationLocked = false;
		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->bOrientRotationToMovement = bSavedOrientToMovement;
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
