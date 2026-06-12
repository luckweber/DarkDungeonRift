// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AttackLight.h"

#include "DDRMotionWarpTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GA_Dash.h"
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

bool UGA_AttackLight::CanActivateAbility(
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

	// Combo de chao so no chao — no ar o GA_AirAttack responde ao mesmo botao (doc 19).
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (ASC->HasMatchingGameplayTag(DDRTags::State_Combat_InAir))
		{
			return false;
		}
	}

	if (const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (const UDDRCombatComponent* Combat = Avatar->FindComponentByClass<UDDRCombatComponent>())
		{
			if (Combat->IsInAirCombat())
			{
				return false;
			}
		}
	}

	return true;
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

	// Dash-attack (estilo Hades): atacar DURANTE o dash (ou na grace logo apos) cancela o
	// dodge e vira a estocada. Detecta pela ABILITY ativa, nao pela tag State.Movement.Dashing
	// — a tag e o i-frame (0.25s do GE) e nao cobre a recovery do dodge. O cancel roda ANTES
	// do lock de rotacao abaixo: o EndAbility do dash restaura bOrientRotationToMovement,
	// e o lock precisa salvar o valor ja restaurado.
	bool bDashAttack = false;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		bool bDashActive = false;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA(UGA_Dash::StaticClass()))
			{
				bDashActive = true;
				break;
			}
		}

		const UDDRCombatComponent* CombatForDash = GetDDRCombatComponent();
		const float TimeSinceDash = CombatForDash ? CombatForDash->GetTimeSinceDashEnded() : FLT_MAX;
		const bool bInGrace = TimeSinceDash <= DashAttackGraceSeconds;
		bDashAttack = RunAttackMontage && (bDashActive || bInGrace);
		bDashAttackOpener = bDashAttack;

		UE_LOG(LogDDR, Log, TEXT("[ATK] %s ATIVADO -> %s (dashAtivo=%d graceT=%.2f<=%.2f? %d) t=%.2f"),
			*GetClass()->GetName(),
			bDashAttack ? TEXT("DASH-ATTACK (estocada)") : TEXT("combo normal"),
			bDashActive ? 1 : 0,
			TimeSinceDash > 999.f ? 999.f : TimeSinceDash, DashAttackGraceSeconds, bInGrace ? 1 : 0,
			GetWorld()->GetTimeSeconds());

		if (bDashActive)
		{
			FGameplayTagContainer DashTags(DDRTags::Ability_Dash);
			ASC->CancelAbilities(&DashTags);
		}
	}

	// Trava orient-to-movement durante o ataque (corpo não gira com WASD no meio do swing).
	// No juggle aéreo o CombatComponent já trava orient + input — não empilhar aqui: o EndAbility
	// restauraria false DEPOIS do UnlockAirHorizontalInput e o chão ficava "foado".
	const UDDRCombatComponent* CombatForOrient = GetDDRCombatComponent();
	const bool bAirInputOwnedByCombat = CombatForOrient && CombatForOrient->IsAirHorizontalInputLocked();

	if (!bAirInputOwnedByCombat)
	{
		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				bSavedOrientToMovement = MoveComp->bOrientRotationToMovement;
				MoveComp->bOrientRotationToMovement = false;
				bRotationLocked = true;
			}
		}
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ResetHitTracking();
		Combat->SetActiveComboSection(ComboIndex);
		if (bDashAttackOpener)
		{
			// AUDITORIA G-01 revertido: dash-attack NÃO usa FaceAndSetupMotionWarp/RunAttack.
			// Hades = estocada reta na direção do dodge (GA_Dash já orientou); avanço = RM de
			// AM_RunAttack. Warp no opener snapava o player pro dummy longe — anti-natural.
			Combat->ClearAttackMotionWarp();
		}
		else
		{
			Combat->FaceAndSetupMotionWarp(GetMotionWarpProfile(), ShouldPreferAirborneTargets());
		}
	}

	if (bDashAttack)
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

	bDashAttackOpener = false;

	if (bRotationLocked)
	{
		bRotationLocked = false;
		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				bool bRestoreOrient = bSavedOrientToMovement;
				// Se o juggle já liberou orient (true) mas salvamos false durante o lock aéreo,
				// nao re-aplicar false — causa walk "foado" ao pousar.
				if (const UDDRCombatComponent* Combat = GetDDRCombatComponent())
				{
					if (!Combat->IsAirHorizontalInputLocked() && !Combat->IsInAirCombat() && !bRestoreOrient)
					{
						bRestoreOrient = true;
					}
				}
				MoveComp->bOrientRotationToMovement = bRestoreOrient;
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
			UE_LOG(LogDDR, Log, TEXT("[ATK] input na JANELA -> avanca t=%.2f"), GetWorld()->GetTimeSeconds());
			TryAdvanceCombo();
			return;
		}

		UE_LOG(LogDDR, Log, TEXT("[ATK] input FORA da janela -> buffer %.2fs t=%.2f"),
			InputBufferSeconds, GetWorld()->GetTimeSeconds());
		Combat->BufferAttackInput(InputBufferSeconds);
	}
}

void UGA_AttackLight::OnMontageEnded()
{
	UE_LOG(LogDDR, Log, TEXT("[ATK] %s montage fim (recovery completa) t=%.2f"),
		*GetClass()->GetName(), GetWorld()->GetTimeSeconds());
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

	UE_LOG(LogDDR, Log, TEXT("[ATK] %s montage INTERROMPIDA (dash/launcher/outro golpe) t=%.2f"),
		*GetClass()->GetName(), GetWorld()->GetTimeSeconds());
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
		UE_LOG(LogDDR, Log, TEXT("[ATK] avanca: estocada -> %s t=%.2f"),
			ComboSections.Num() > 0 ? *ComboSections[0].ToString() : TEXT("?"),
			GetWorld()->GetTimeSeconds());
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
		UE_LOG(LogDDR, Log, TEXT("[ATK] sem proxima secao (fim do combo em %s)"),
			*ComboSections[ComboIndex].ToString());
		return;
	}

	UE_LOG(LogDDR, Log, TEXT("[ATK] avanca: %s -> %s t=%.2f"),
		*ComboSections[ComboIndex].ToString(), *ComboSections[NextIndex].ToString(),
		GetWorld()->GetTimeSeconds());

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
	// Dash-attack: sem face/warp — estocada reta na direcao do dodge (root motion de AM_RunAttack).
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
