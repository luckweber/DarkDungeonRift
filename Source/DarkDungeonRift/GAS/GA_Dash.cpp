// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Dash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GE_DDRCooldownDash.h"
#include "GE_DDRDashIFrames.h"
#include "GameFramework/Character.h"

UGA_Dash::UGA_Dash()
{
	AbilityTags.AddTag(DDRTags::Ability_Dash);

	// NAO por State.Movement.Dashing em ActivationOwnedTags: a tag e a janela de I-FRAME
	// (concedida pelo GE_DDRDashIFrames por 0.25s, doc 19 §3) — amarra-la a vida da ability
	// estenderia a invencibilidade pra recovery toda do dodge. O dash-attack detecta o dash
	// pela ability ativa (GA_AttackLight) + grace do NotifyDashEnded.
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

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		// Trava a rotação: o corpo NÃO gira no meio do dodge (orient-to-movement giraria
		// pra direção do input durante o deslocamento). Restaurada no EndAbility.
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			bSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
			MoveComp->bOrientRotationToMovement = false;
			bModifiedRotationFlag = true;
		}

		// "Modo Hades": gira pra direção do dash e usa só a seção F.
		if (bFaceDashDirection && !Direction.IsNearlyZero())
		{
			FRotator FaceRot = Direction.Rotation();
			FaceRot.Pitch = 0.f;
			FaceRot.Roll = 0.f;
			Character->SetActorRotation(FaceRot);
		}
	}

	const FName Section = ComputeDodgeSectionName(Direction);

	UE_LOG(LogDDR, Log, TEXT("[DASH] ATIVADO dir=(%.2f, %.2f) secao=%s modo=%s t=%.2f"),
		Direction.X, Direction.Y, *Section.ToString(),
		(DashMontage && DashMontage->HasRootMotion()) ? TEXT("RM clip") : TEXT("ConstantForce"),
		GetWorld()->GetTimeSeconds());

	// Dois modos, AUTO-DETECTADOS (doc 59 §3):
	//  A) Clips COM root motion  -> o CLIP dirige a cápsula (distância/easing autorados).
	//  B) Clips in-place / sem montage -> ConstantForce dirige (DashDistance/DashDuration).
	if (DashMontage && DashMontage->HasRootMotion())
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DashMontage,
			1.f,
			Section,
			/*bStopWhenAbilityEnds=*/false,
			RootMotionTranslationScale);
		MontageTask->OnCompleted.AddDynamic(this, &UGA_Dash::OnDashMontageFinished);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Dash::OnDashMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Dash::OnDashMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Dash::OnDashMontageFinished);
		MontageTask->ReadyForActivation();
		return;
	}

	PlayVisualDodgeMontage(Section);

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

void UGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Restaura a rotação (o controle volta; a montage segue o recovery visual e é
	// interrompível pelo slot — dash-cancel→attack corta o dodge naturalmente).
	if (bModifiedRotationFlag)
	{
		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->bOrientRotationToMovement = bSavedOrientRotationToMovement;
			}
		}
		bModifiedRotationFlag = false;
	}

	// Abre a janela de grace do dash-attack (LMB logo apos o dash ainda vira a estocada).
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->NotifyDashEnded();
	}

	UE_LOG(LogDDR, Log, TEXT("[DASH] fim cancelled=%d t=%.2f%s"),
		bWasCancelled ? 1 : 0,
		GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f,
		bWasCancelled ? TEXT(" (cancelado — dash-attack ou morte)") : TEXT(""));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dash::OnRootMotionFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dash::OnDashMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FName UGA_Dash::ComputeDodgeSectionName(const FVector& DashDirection) const
{
	if (bFaceDashDirection)
	{
		return TEXT("F");
	}

	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return TEXT("F");
	}

	// Ângulo assinado entre o facing atual e a direção do dash (0 = frente).
	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Dir2D = DashDirection.GetSafeNormal2D();
	if (Forward.IsNearlyZero() || Dir2D.IsNearlyZero())
	{
		return TEXT("F");
	}

	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(
		FVector::CrossProduct(Forward, Dir2D).Z,
		FVector::DotProduct(Forward, Dir2D)));
	const FName BaseSection = GetDirectionalSectionName(Angle);

	// Intenção de continuar correndo? -> família "to_Run" (se a seção existir na montage).
	const bool bWantsToKeepMoving = !Character->GetLastMovementInputVector().IsNearlyZero();
	if (bWantsToKeepMoving && DashMontage && !RunSectionSuffix.IsNone())
	{
		const FName RunSection(*(BaseSection.ToString() + RunSectionSuffix.ToString()));
		if (DashMontage->GetSectionIndex(RunSection) != INDEX_NONE)
		{
			return RunSection;
		}
	}

	return BaseSection;
}

void UGA_Dash::PlayVisualDodgeMontage(FName SectionName)
{
	// Modo B: a montage é SÓ visual (clips in-place) — o deslocamento vem do ConstantForce.
	if (!DashMontage)
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInstance = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Play(DashMontage, 1.f);
	if (DashMontage->GetSectionIndex(SectionName) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(SectionName, DashMontage);
	}
}

FName UGA_Dash::GetDirectionalSectionName(float AngleDegrees)
{
	// 8 setores de 45°, relativos ao facing (0 = frente, +90 = direita, ±180 = trás).
	const float A = FRotator::NormalizeAxis(AngleDegrees);
	if (A >= -22.5f && A < 22.5f)    return TEXT("F");
	if (A >= 22.5f && A < 67.5f)     return TEXT("FR");
	if (A >= 67.5f && A < 112.5f)    return TEXT("R");
	if (A >= 112.5f && A < 157.5f)   return TEXT("BR");
	if (A >= -67.5f && A < -22.5f)   return TEXT("FL");
	if (A >= -112.5f && A < -67.5f)  return TEXT("L");
	if (A >= -157.5f && A < -112.5f) return TEXT("BL");
	return TEXT("B");
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
