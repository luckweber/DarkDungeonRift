// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Dash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRGameplayTags.h"
#include "GE_DDRCooldownDash.h"
#include "GE_DDRDashIFrames.h"
#include "GameFramework/Character.h"

UGA_Dash::UGA_Dash()
{
	AbilityTags.AddTag(DDRTags::Ability_Dash);

	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(DDRTags::Cooldown_Dash);
	ActivationBlockedTags.AppendTags(BlockedTags);

	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(DDRTags::Ability_Attack);
	CancelAbilitiesWithTag = CancelTags;

	IFrameEffectClass = UGE_DDRDashIFrames::StaticClass();
	CooldownEffectClass = UGE_DDRCooldownDash::StaticClass();
	CooldownGameplayEffectClass = UGE_DDRCooldownDash::StaticClass();
}

void UGA_Dash::ActivateAbility(
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

	if (IFrameEffectClass)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, IFrameEffectClass.GetDefaultObject(), 1.f, 1);
	}

	const FVector Direction = GetDashDirection();
	const float Strength = DashDistance / FMath::Max(DashDuration, KINDA_SMALL_NUMBER);

	UAbilityTask_ApplyRootMotionConstantForce* RootMotionTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		NAME_None,
		Direction,
		Strength,
		DashDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		1.f,
		false);
	RootMotionTask->OnFinish.AddDynamic(this, &UGA_Dash::OnRootMotionFinished);
	RootMotionTask->ReadyForActivation();
}

void UGA_Dash::OnRootMotionFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FVector UGA_Dash::GetDashDirection() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	FVector Direction = Character->GetLastMovementInputVector();
	if (Direction.IsNearlyZero())
	{
		if (const UDDRCharacterMovementComponent* DDRMove = Cast<UDDRCharacterMovementComponent>(Character->GetCharacterMovement()))
		{
			Direction = DDRMove->GetLastInputVector();
		}
	}

	if (Direction.IsNearlyZero())
	{
		Direction = Character->GetActorForwardVector();
	}

	Direction.Z = 0.f;
	return Direction.GetSafeNormal();
}
