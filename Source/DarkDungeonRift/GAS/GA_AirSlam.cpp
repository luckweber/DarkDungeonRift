// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AirSlam.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DDRCharacterBase.h"
#include "DDRCombatComponent.h"
#include "DDRCombatTypes.h"
#include "DDRHitStopSubsystem.h"
#include "DDRMotionWarpTypes.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	bImpactTriggered = false;

	UE_LOG(LogDDR, Log, TEXT("[SLAM] ATIVADO montage=%s | secoes: Start=%d Loop=%d End=%d | playerZ=%.0f t=%.2f"),
		*GetNameSafe(SlamMontage),
		SlamMontage->GetSectionIndex(StartSection),
		SlamMontage->GetSectionIndex(LoopSection),
		SlamMontage->GetSectionIndex(LandSection),
		Character->GetActorLocation().Z,
		GetWorld()->GetTimeSeconds());

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

	// Fase de queda: Start -> Loop e Loop -> Loop (self-loop) via codigo — as secoes da
	// montage ficam SEM link no editor (regra do doc 57). A queda dura o que durar;
	// o LandedDelegate pula pra End. Sem secao "Loop" na montage = MVP 2 secoes, segue valido.
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			if (SlamMontage->GetSectionIndex(LoopSection) != INDEX_NONE)
			{
				AnimInstance->Montage_SetNextSection(StartSection, LoopSection, SlamMontage);
				AnimInstance->Montage_SetNextSection(LoopSection, LoopSection, SlamMontage);
			}

			// Blindagem: RM ON em clip de queda faria a anim dirigir a capsula e ANULAR a
			// velocity -3500 (player "boiando" — o hold do alvo expira e o slam erra a
			// coreografia). Durante o slam o RM e ignorado; restaurado no EndAbility.
			SavedRootMotionMode = static_cast<uint8>(AnimInstance->RootMotionMode);
			AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
			bSavedRootMotionMode = true;
		}
	}

	// Sai do hold aereo e DESCE com forca (16 par.5). O alvo juggleado fica SEGURO no ar
	// (ExitAirCombat estende o hold dele) ate o impacto derrubar.
	AActor* SlamTarget = nullptr;
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		SlamTarget = Combat->FaceAndSetupMotionWarp(EDDRMotionWarpProfile::Slam, /*bPreferAirborne=*/true);
		Combat->ExitAirCombat(/*bSlam=*/true);
	}

	// A descida e DO SLAM, incondicional. Confiar so no ExitAirCombat deixava um buraco:
	// R DURANTE a montage do launcher (tag InAir entra no hit; EnterAirCombat so no fim)
	// -> ExitAirCombat no-op -> player ficava com a velocity de SUBIDA do launcher e
	// fazia um arco balistico de ~1.2s preso no Loop (dummy ja no chao).
	if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Falling);
		FVector Vel = MoveComp->Velocity;
		Vel.Z = SlamFallVelocity;
		MoveComp->Velocity = Vel;
	}

	{
		const ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(SlamTarget);
		UE_LOG(LogDDR, Log, TEXT("[SLAM] alvo soft-lock=%s airborne=%d dist2D=%.0f dz=%.0f"),
			*GetNameSafe(SlamTarget),
			(TargetChar && TargetChar->IsAirborne()) ? 1 : 0,
			SlamTarget ? FVector::Dist2D(SlamTarget->GetActorLocation(), Character->GetActorLocation()) : -1.f,
			SlamTarget ? SlamTarget->GetActorLocation().Z - Character->GetActorLocation().Z : 0.f);
	}

	// Golpe do APEX: arremessa o alvo JA na ativacao — ele desce mais rapido (-4500) que
	// o player (-3500) e esmaga no chao primeiro. Beat de hit-stop vende o "agarrao".
	if (bSlamClaimedTargetOnActivate)
	{
		if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(SlamTarget))
		{
			if (TargetChar->IsAirborne())
			{
				UE_LOG(LogDDR, Log, TEXT("[SLAM] APEX: arremessando %s pro chao (antes do player)"),
					*GetNameSafe(TargetChar));
				TargetChar->EndAirborne(/*bSlammed=*/true);

				if (SlamGrabHitStopFrames > 0)
				{
					if (UDDRHitStopSubsystem* HitStop = GetWorld()->GetSubsystem<UDDRHitStopSubsystem>())
					{
						HitStop->RequestHitStop(SlamGrabHitStopFrames);
					}
				}

				if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
				{
					FGameplayCueParameters CueParams;
					CueParams.Instigator = Character;
					CueParams.Location = TargetChar->GetActorLocation();
					ASC->ExecuteGameplayCue(DDRTags::Cue_Hit_Light, CueParams);
				}
			}
		}
	}

	// Homing: mira o XY do alvo pra coluna do impacto SEMPRE conectar (cap = whiff honesto).
	if (bHomeToTarget && SlamTarget)
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			FVector To = SlamTarget->GetActorLocation() - Character->GetActorLocation();
			To.Z = 0.f;
			const float Dist2D = To.Size();
			if (Dist2D > KINDA_SMALL_NUMBER && Dist2D <= SlamHomingMaxDistance)
			{
				// Tempo de queda estimado pela altura real ate o chao (trace) e velocity do exit.
				FHitResult Floor;
				FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(DDRSlamFloor), false, Character);
				const FVector TraceStart = Character->GetActorLocation();
				GetWorld()->LineTraceSingleByChannel(
					Floor, TraceStart, TraceStart - FVector(0.f, 0.f, 4000.f), ECC_Visibility, FloorParams);

				const float FallDist = Floor.bBlockingHit
					? FMath::Max(TraceStart.Z - Floor.ImpactPoint.Z, 100.f)
					: 400.f;
				const float FallSpeed = FMath::Max(-MoveComp->Velocity.Z, 500.f);
				const float FallTime = FMath::Max(FallDist / FallSpeed, 0.05f);

				FVector VelXY = To / FallTime;
				VelXY = VelXY.GetClampedToMaxSize(SlamMaxHorizontalSpeed);
				MoveComp->Velocity = FVector(VelXY.X, VelXY.Y, MoveComp->Velocity.Z);

				UE_LOG(LogDDR, Log, TEXT("[SLAM] homing ON velXY=(%.0f, %.0f) quedaDist=%.0f quedaT=%.2fs velZ=%.0f"),
					VelXY.X, VelXY.Y, FallDist, FallTime, MoveComp->Velocity.Z);
			}
			else
			{
				UE_LOG(LogDDR, Log, TEXT("[SLAM] homing OFF (dist2D=%.0f > cap %.0f ou zero) — desce reto velZ=%.0f"),
					Dist2D, SlamHomingMaxDistance, MoveComp->Velocity.Z);
			}
		}
	}
}

void UGA_AirSlam::OnSlamLanded(const FHitResult& Hit)
{
	if (bImpactTriggered)
	{
		return;
	}
	bImpactTriggered = true;

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	UE_LOG(LogDDR, Log, TEXT("[SLAM] POUSO em (%.0f, %.0f, %.0f) t=%.2f — disparando AoE"),
		Character ? Character->GetActorLocation().X : 0.f,
		Character ? Character->GetActorLocation().Y : 0.f,
		Character ? Character->GetActorLocation().Z : 0.f,
		GetWorld()->GetTimeSeconds());

	// Anim: pula pra fase de impacto (sai do Loop/Start).
	bool bJumpedToEnd = false;
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
					bJumpedToEnd = true;
				}
			}
		}
	}

	// AoE no ponto de impacto: COLUNA vertical (raio x SlamVerticalReach) — dano +
	// derruba os Airborne (inclusive o juggleado la no alto) + hit-stop pesado.
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		FDDRMeleeSweepParams AoE;
		AoE.BaseDamage = SlamDamage;
		AoE.SweepRadius = SlamRadius;
		AoE.SweepReach = SlamVerticalReach;
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

	UE_LOG(LogDDR, Log, TEXT("[SLAM] impacto resolvido: jumpToEnd=%d"), bJumpedToEnd ? 1 : 0);

	// Montage ja tinha acabado (queda mais longa que o clip, sem secao Loop): o pouso
	// fez o AoE — encerra aqui, porque OnMontageFinished nao vira de novo.
	if (!bJumpedToEnd)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_AirSlam::OnMontageFinished()
{
	// Clip acabou ANTES do pouso (montage sem Loop em queda alta): nao encerra — o
	// impacto/AoE pertence ao LandedDelegate; ele encerra a ability quando pousar.
	if (!bImpactTriggered)
	{
		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling())
			{
				UE_LOG(LogDDR, Log, TEXT("[SLAM] montage acabou AINDA CAINDO (sem Loop?) — aguardando pouso"));
				return;
			}
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] fim (montage encerrou; impacto=%d)"), bImpactTriggered ? 1 : 0);
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

	if (bSavedRootMotionMode)
	{
		bSavedRootMotionMode = false;
		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->SetRootMotionMode(static_cast<ERootMotionMode::Type>(SavedRootMotionMode));
				}
			}
		}
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->ClearAttackMotionWarp();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
