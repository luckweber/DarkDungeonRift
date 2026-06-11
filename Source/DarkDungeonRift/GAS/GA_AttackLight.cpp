// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AttackLight.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_AttackLight::UGA_AttackLight()
{
	AbilityTags.AddTag(DDRTags::Ability_Attack);
	AbilityTags.AddTag(DDRTags::Ability_Attack_Light);

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(DDRTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;

	// No ar, quem atende o MESMO botao e o GA_AirAttack (gate por tag, doc 19).
	ActivationBlockedTags.AddTag(DDRTags::State_Combat_InAir);

	bRetriggerInstancedAbility = false;
}

void UGA_AttackLight::ActivateAbility(
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

	if (!ComboMontage)
	{
		UE_LOG(LogDDR, Warning, TEXT("GA_AttackLight: ComboMontage not assigned on %s"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboIndex = 0;
	bPlayingRunAttack = false;
	bIgnoreNextInterrupt = false;

	// Trava a rotação durante o combo: o facing do SOFT-LOCK manda (WASD não gira o corpo
	// no meio do golpe). Restaurada no EndAbility.
	if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
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
		Combat->SetActiveComboSection(ComboIndex);
		Combat->FaceAndSetupMotionWarp(GetMotionWarpProfile(), ShouldPreferAirborneTargets());
	}

	// Opener em corrida: combo COMECANDO + velocidade alta + montage setada.
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const float Speed2D = Character ? Character->GetVelocity().Size2D() : 0.f;
	if (RunAttackMontage && Speed2D >= RunAttackMinSpeed)
	{
		bPlayingRunAttack = true;
		PlayRunAttackOpener();
	}
	else
	{
		PlayCurrentSection();
	}

	UAbilityTask_WaitGameplayEvent* WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		DDRTags::Event_Combat_Hit,
		nullptr,
		false,
		false);
	WaitHitTask->EventReceived.AddDynamic(this, &UGA_AttackLight::OnHitEvent);
	WaitHitTask->ReadyForActivation();
}

void UGA_AttackLight::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->CloseComboWindow();
		Combat->ClearBufferedAttack();
		Combat->ClearAttackMotionWarp();
	}

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

	ComboIndex = 0;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_AttackLight::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		if (Combat->IsComboWindowOpen())
		{
			TryAdvanceCombo();
			return;
		}

		Combat->BufferAttackInput(InputBufferSeconds);
	}
}

void UGA_AttackLight::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_AttackLight::OnMontageCancelled()
{
	// Troca opener->combo dispara OnInterrupted da task antiga — nao e um cancel real.
	if (bIgnoreNextInterrupt)
	{
		bIgnoreNextInterrupt = false;
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_AttackLight::OnHitEvent(FGameplayEventData Payload)
{
	// Hit resolution is handled by UDDRCombatComponent when the notify sweeps.
}

void UGA_AttackLight::TryAdvanceCombo()
{
	if (ComboSections.Num() == 0)
	{
		return;
	}

	// Saindo do opener de corrida -> entra no combo normal (Atk1).
	if (bPlayingRunAttack)
	{
		bPlayingRunAttack = false;
		ComboIndex = 0;
		bIgnoreNextInterrupt = true; // Montage_Play novo interrompe a task do opener

		if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
		{
			Combat->ResetHitTracking();
			Combat->SetActiveComboSection(ComboIndex);
			Combat->CloseComboWindow();
			Combat->ClearBufferedAttack();
		}

		PlayCurrentSection();
		return;
	}

	const int32 NextIndex = ComboIndex + 1;
	if (NextIndex >= ComboSections.Num())
	{
		return;
	}

	ComboIndex = NextIndex;

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ResetHitTracking();
		Combat->SetActiveComboSection(ComboIndex);
		Combat->CloseComboWindow();
		Combat->ClearBufferedAttack();
		Combat->FaceAndSetupMotionWarp(GetMotionWarpProfile(), ShouldPreferAirborneTargets());
	}

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(ComboMontage))
				{
					const FName SectionName = ComboSections[FMath::Clamp(ComboIndex, 0, ComboSections.Num() - 1)];
					AnimInstance->Montage_JumpToSection(SectionName, ComboMontage);
					return;
				}
			}
		}
	}

	PlayCurrentSection();
}

void UGA_AttackLight::PlayCurrentSection()
{
	if (!ComboMontage || ComboSections.Num() == 0)
	{
		return;
	}

	const FName SectionName = ComboSections[FMath::Clamp(ComboIndex, 0, ComboSections.Num() - 1)];

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		ComboMontage,
		1.f,
		SectionName,
		true);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_AttackLight::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_AttackLight::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_AttackLight::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_AttackLight::PlayRunAttackOpener()
{
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->FaceAndSetupMotionWarp(EDDRMotionWarpProfile::RunAttack, ShouldPreferAirborneTargets());
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		RunAttackMontage,
		1.f,
		NAME_None,
		true);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_AttackLight::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_AttackLight::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_AttackLight::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}
