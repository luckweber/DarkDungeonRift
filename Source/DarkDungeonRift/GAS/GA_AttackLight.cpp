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

UGA_AttackLight::UGA_AttackLight()
{
	AbilityTags.AddTag(DDRTags::Ability_Attack);
	AbilityTags.AddTag(DDRTags::Ability_Attack_Light);

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(DDRTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;

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

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		if (Combat->IsComboWindowOpen())
		{
			TryAdvanceCombo();
		}
		else
		{
			ComboIndex = 0;
		}
		Combat->ResetHitTracking();
		Combat->SetActiveComboSection(ComboIndex);
	}
	else
	{
		ComboIndex = 0;
	}

	PlayCurrentSection();

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
