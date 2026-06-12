// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Launcher.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "DDRCombatComponent.h"
#include "DDRMotionWarpTypes.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GE_DDRCooldownLauncher.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Launcher::UGA_Launcher()
{
	AbilityTags.AddTag(DDRTags::Ability_Attack);
	AbilityTags.AddTag(DDRTags::Ability_Attack_Launcher);

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(DDRTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;

	// Nao relanca no ar; respeita o cooldown.
	ActivationBlockedTags.AddTag(DDRTags::State_Combat_InAir);
	ActivationBlockedTags.AddTag(DDRTags::Cooldown_Launcher);

	// Attack -> Launcher (cancelamento-ancora, 15 par.6): corta o combo de chao.
	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(DDRTags::Ability_Attack_Light);
	CancelAbilitiesWithTag = CancelTags;

	CooldownGameplayEffectClass = UGE_DDRCooldownLauncher::StaticClass();
}

bool UGA_Launcher::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UDDRCombatComponent* Combat = Avatar ? Avatar->FindComponentByClass<UDDRCombatComponent>() : nullptr;
	if (!Combat)
	{
		return false;
	}

	const AActor* Target = Combat->FindSoftLockTarget(/*bPreferAirborne=*/false);
	if (!Target)
	{
		return false;
	}

	if (!UDDRCombatComponent::CanLaunchActor(Target))
	{
		return false;
	}

	const float Dist2D = FVector::Dist2D(Target->GetActorLocation(), Avatar->GetActorLocation());
	return Dist2D <= LauncherMaxActivationDistance;
}

void UGA_Launcher::ActivateAbility(
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

	if (!LauncherMontage)
	{
		UE_LOG(LogDDR, Warning, TEXT("GA_Launcher: LauncherMontage not assigned on %s"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ResetHitTracking();
		Combat->ApplyLauncherAirTuning(LaunchRiseHeight, JuggleTargetHeightAbovePlayer);
		Combat->FaceAndSetupMotionWarp(EDDRMotionWarpProfile::Launcher, /*bPreferAirborne=*/false);
	}

	// FIX "root lock": em MOVE_Walking o CMC descarta o Z do root motion (o player dava
	// um pulinho e travava no chao). Flying durante a montage deixa o clip Floor_To_Air
	// subir a capsula de verdade. Restaurado no EndAbility se nao entrar no ar.
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_Flying);
			bSetFlyingForLaunch = true;

			if (UDDRCombatComponent* CombatForLock = GetDDRCombatComponent())
			{
				CombatForLock->LockAirHorizontalInput();
			}
		}
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, LauncherMontage, 1.f, NAME_None, /*bStopWhenAbilityEnds=*/false);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Launcher::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Launcher::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Launcher::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Launcher::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Launcher::OnMontageCompleted()
{
	// Follow launch (16 par.2): so entra no ar se LANCOU alguem (whiff = fica no chao).
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		if (Combat->DidLaunchTargetThisSwing())
		{
			Combat->EnterAirCombat();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Launcher::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Launcher::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Whiff/cancel: volta a cair (gravidade resolve o pouso). Se EnterAirCombat rodou,
	// o hold aereo e dono do Flying — nao mexer.
	if (bSetFlyingForLaunch)
	{
		bSetFlyingForLaunch = false;

		const UDDRCombatComponent* Combat = GetDDRCombatComponent();
		const bool bHeldInAir = Combat && Combat->IsInAirCombat();
		if (!bHeldInAir)
		{
			if (UDDRCombatComponent* CombatForUnlock = GetDDRCombatComponent())
			{
				CombatForUnlock->UnlockAirHorizontalInput();
			}

			if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
			{
				if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
				{
					if (MoveComp->MovementMode == MOVE_Flying)
					{
						MoveComp->SetMovementMode(MOVE_Falling);
					}
				}
			}

			// Cancel POS-hit: LaunchTarget ja tinha posto a tag InAir, mas EnterAirCombat
			// nao rodou — sem isto a tag ficaria presa (ExitAirCombat early-returns).
			if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
			{
				ASC->SetLooseGameplayTagCount(DDRTags::State_Combat_InAir, 0);
			}
		}
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ClearAttackMotionWarp();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
