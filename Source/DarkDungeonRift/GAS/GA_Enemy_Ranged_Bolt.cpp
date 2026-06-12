// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Enemy_Ranged_Bolt.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DDREnemyAIController.h"
#include "DDREnemyTypes.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "DDRProjectileBase.h"
#include "DDRProjectileData.h"
#include "DDRProjectilePoolSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Enemy_Ranged_Bolt::UGA_Enemy_Ranged_Bolt()
{
	AbilityTags.AddTag(DDRTags::Ability_Enemy_Ranged_Bolt);

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(DDRTags::State_Combat_Attacking);
	ActivationOwnedTags = OwnedTags;

	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(DDRTags::State_Combat_Airborne);
	BlockedTags.AddTag(DDRTags::State_Combat_Stagger);
	ActivationBlockedTags.AppendTags(BlockedTags);
}

void UGA_Enemy_Ranged_Bolt::ActivateAbility(
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

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			bSavedOrientToMovement = MoveComp->bOrientRotationToMovement;
			MoveComp->bOrientRotationToMovement = false;
			bRotationLocked = true;
		}

		if (AActor* Target = GetAggroTarget())
		{
			const FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				FRotator FaceRot = ToTarget.Rotation();
				FaceRot.Pitch = 0.f;
				FaceRot.Roll = 0.f;
				Character->SetActorRotation(FaceRot);
			}
		}
	}

	PlayTelegraphCue();

	const float Windup = ProjectileData ? ProjectileData->TelegraphDuration : 0.55f;
	UAbilityTask_WaitDelay* WindupTask = UAbilityTask_WaitDelay::WaitDelay(this, Windup);
	WindupTask->OnFinish.AddDynamic(this, &UGA_Enemy_Ranged_Bolt::OnWindupFinished);
	WindupTask->ReadyForActivation();
}

void UGA_Enemy_Ranged_Bolt::OnWindupFinished()
{
	SpawnBolt();

	if (RecoverySeconds > KINDA_SMALL_NUMBER)
	{
		UAbilityTask_WaitDelay* RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, RecoverySeconds);
		RecoveryTask->OnFinish.AddDynamic(this, &UGA_Enemy_Ranged_Bolt::OnRecoveryFinished);
		RecoveryTask->ReadyForActivation();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Enemy_Ranged_Bolt::OnRecoveryFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

AActor* UGA_Enemy_Ranged_Bolt::GetAggroTarget() const
{
	const APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Avatar)
	{
		return nullptr;
	}

	if (const ADDREnemyAIController* AI = Cast<ADDREnemyAIController>(Avatar->GetController()))
	{
		if (const UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			return Cast<AActor>(BB->GetValueAsObject(DDRBlackboardKeys::TargetActor));
		}
	}

	return nullptr;
}

void UGA_Enemy_Ranged_Bolt::SpawnBolt()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	AActor* Target = GetAggroTarget();
	if (!Avatar || !ProjectileData)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Avatar->FindComponentByClass<USkeletalMeshComponent>();
	const FTransform SpawnTM = Mesh
		? Mesh->GetSocketTransform(MuzzleSocketName)
		: Avatar->GetActorTransform();

	FVector Dir = Target
		? (Target->GetActorLocation() - SpawnTM.GetLocation())
		: Avatar->GetActorForwardVector();
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();

	UDDRProjectilePoolSubsystem* Pool = GetWorld()->GetSubsystem<UDDRProjectilePoolSubsystem>();
	if (!Pool)
	{
		return;
	}

	ADDRProjectileBase* Projectile = Pool->Acquire(ProjectileClass, SpawnTM);
	if (Projectile)
	{
		Projectile->InitProjectile(ProjectileData, Avatar, Dir);
	}
}

void UGA_Enemy_Ranged_Bolt::PlayTelegraphCue()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetAvatarActorFromActorInfo();
		if (AActor* Target = GetAggroTarget())
		{
			CueParams.Location = Target->GetActorLocation();
		}
		ASC->ExecuteGameplayCue(DDRTags::Cue_Proj_Telegraph, CueParams);
	}
}

void UGA_Enemy_Ranged_Bolt::EndAbility(
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
