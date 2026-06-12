// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_AirSlam.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DDRCharacterBase.h"
#include "DDRCombatComponent.h"
#include "DDRHitStopSubsystem.h"
#include "DDRMotionWarpTypes.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

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
	bEndSectionStarted = false;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(DDRTags::State_Combat_SlamFall);
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] ATIVADO montage=%s | secoes: Start=%d Loop=%d End=%d | playerZ=%.0f endTrigger=%d t=%.2f"),
		*GetNameSafe(SlamMontage),
		SlamMontage->GetSectionIndex(StartSection),
		SlamMontage->GetSectionIndex(LoopSection),
		SlamMontage->GetSectionIndex(LandSection),
		Character->GetActorLocation().Z,
		static_cast<int32>(SlamEndTrigger),
		GetWorld()->GetTimeSeconds());

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		Combat->RegisterSlamAbility(this);
	}

	// Pouso: cue / End so se SlamEndTrigger == OnGroundLand.
	Character->LandedDelegate.AddDynamic(this, &UGA_AirSlam::OnSlamLanded);
	bLandedBound = true;

	if (SlamEndTrigger == EDDRSlamEndTrigger::BeforeGroundProximity)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SlamProximityTimerHandle,
			this,
			&UGA_AirSlam::CheckSlamEndProximity,
			0.016f,
			true);
		CheckSlamEndProximity();
	}

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

	// MARCO T3 (doc 16 §5): sai do hold aéreo do player; alvo fica no ar (ExtendAirborneHold).
	// Perfil Slam = só encara (sem warp XY). ExitAirCombat exige bInAirCombat — se R cortou o
	// launcher antes do T1, o no-op abaixo é normal; a velocity abaixo ainda inicia a queda.
	AActor* SlamTarget = nullptr;
	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		SlamTarget = Combat->FaceAndSetupMotionWarp(EDDRMotionWarpProfile::Slam, /*bPreferAirborne=*/true);
		Combat->ExitAirCombat(/*bSlam=*/true);
	}

	// Descida incondicional (não delegar só ao ExitAirCombat). Buraco histórico:
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

void UGA_AirSlam::JumpToMontageSection(const FName SectionName)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !SlamMontage || SectionName.IsNone())
	{
		return;
	}

	if (SlamMontage->GetSectionIndex(SectionName) == INDEX_NONE)
	{
		UE_LOG(LogDDR, Warning, TEXT("[SLAM] secao '%s' nao existe em %s"), *SectionName.ToString(), *GetNameSafe(SlamMontage));
		return;
	}

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(SlamMontage))
			{
				AnimInstance->Montage_JumpToSection(SectionName, SlamMontage);
				if (SectionName == LandSection)
				{
					bEndSectionStarted = true;
				}
				UE_LOG(LogDDR, Log, TEXT("[SLAM] JumpToSection '%s' z=%.0f t=%.2f"),
					*SectionName.ToString(), Character->GetActorLocation().Z, GetWorld()->GetTimeSeconds());
			}
		}
	}
}

void UGA_AirSlam::ResumeFallAfterSlamPin(const float FallVelocityZ, const bool bResumeLoopSection)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	const bool bWasPinnedFlying = MoveComp->MovementMode == MOVE_Flying;
	if (!MoveComp->IsFalling() && !bWasPinnedFlying)
	{
		UE_LOG(LogDDR, Log, TEXT("[SLAM] retoma queda IGNORADA — no chao endStarted=%d t=%.2f"),
			bEndSectionStarted ? 1 : 0, GetWorld()->GetTimeSeconds());
		return;
	}

	MoveComp->SetMovementMode(MOVE_Falling);
	FVector Vel = MoveComp->Velocity;
	Vel.Z = FallVelocityZ;
	MoveComp->Velocity = Vel;

	// Nunca volta pra Loop depois que End começou (BeforeGroundProximity / hit) — causa loop infinito.
	const bool bShouldResumeLoop = bResumeLoopSection && bResumeLoopSectionAfterPin
		&& !bEndSectionStarted
		&& SlamMontage->GetSectionIndex(LoopSection) != INDEX_NONE;

	if (bShouldResumeLoop)
	{
		JumpToMontageSection(LoopSection);
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->Montage_SetNextSection(LoopSection, LoopSection, SlamMontage);
			}
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] retoma queda velZ=%.0f loop=%d endStarted=%d t=%.2f"),
		FallVelocityZ, bShouldResumeLoop ? 1 : 0, bEndSectionStarted ? 1 : 0, GetWorld()->GetTimeSeconds());
}

void UGA_AirSlam::NotifySlamHitboxJumpToEnd()
{
	TryJumpToEndSection(TEXT("Hitbox"));
}

void UGA_AirSlam::TryJumpToEndSection(const TCHAR* Reason)
{
	// Único lugar que chama BeginSlamAirPin (audit S-08). NÃO mover para ANS_DDRHitbox::
	// NotifyBegin — o pin no início do hitbox chega depois do pouso físico (-3500 em ~1 frame).
	if (bEndSectionStarted)
	{
		return;
	}

	bEndSectionStarted = true;
	GetWorld()->GetTimerManager().ClearTimer(SlamProximityTimerHandle);

	// Congela ANTES do jump de secao — velocity -3500 pousa em ~1 frame; o notify do
	// hitbox chega tarde demais (log: POUSO 0.01s antes do PinInAir).
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			if (MoveComp->IsFalling())
			{
				if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
				{
					Combat->SnapSlamEndToJuggleTarget();
					Combat->BeginSlamAirPin();
				}
			}
		}
	}

	JumpToMontageSection(LandSection);

	if (!bImpactTriggered)
	{
		bImpactTriggered = true;
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.Instigator = GetAvatarActorFromActorInfo();
			if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
			{
				CueParams.Location = Character->GetActorLocation();
			}
			ASC->ExecuteGameplayCue(DDRTags::Cue_Slam, CueParams);
		}

		if (UDDRHitStopSubsystem* HitStop = GetWorld()->GetSubsystem<UDDRHitStopSubsystem>())
		{
			HitStop->RequestHitStop(SlamHitStopFrames);
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] End via %s (impacto declarado)"), Reason);
}

void UGA_AirSlam::CheckSlamEndProximity()
{
	if (bEndSectionStarted || SlamEndTrigger != EDDRSlamEndTrigger::BeforeGroundProximity)
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !MoveComp || !MoveComp->IsFalling())
	{
		return;
	}

	FHitResult Floor;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DDRSlamEndProx), false, Character);
	const FVector Loc = Character->GetActorLocation();
	if (!GetWorld()->LineTraceSingleByChannel(
		Floor, Loc, Loc - FVector(0.f, 0.f, 4000.f), ECC_Visibility, Params))
	{
		return;
	}

	const float DistToGround = Loc.Z - Floor.ImpactPoint.Z;
	if (DistToGround <= SlamEndGroundProximity)
	{
		GetWorld()->GetTimerManager().ClearTimer(SlamProximityTimerHandle);
		TryJumpToEndSection(TEXT("BeforeGroundProximity"));
	}
}

void UGA_AirSlam::OnSlamLanded(const FHitResult& Hit)
{
	GetWorld()->GetTimerManager().ClearTimer(SlamProximityTimerHandle);

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		if (Combat->IsSlamAirPinActive())
		{
			UE_LOG(LogDDR, Log, TEXT("[SLAM] POUSO ignorado — PinInAir ativo z=%.0f t=%.2f"),
				Character ? Character->GetActorLocation().Z : 0.f, GetWorld()->GetTimeSeconds());
			return;
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] POUSO em (%.0f, %.0f, %.0f) endStarted=%d t=%.2f"),
		Character ? Character->GetActorLocation().X : 0.f,
		Character ? Character->GetActorLocation().Y : 0.f,
		Character ? Character->GetActorLocation().Z : 0.f,
		bEndSectionStarted ? 1 : 0,
		GetWorld()->GetTimeSeconds());

	if (SlamEndTrigger == EDDRSlamEndTrigger::OnGroundLand && !bEndSectionStarted)
	{
		TryJumpToEndSection(TEXT("OnGroundLand"));
	}

	if (!bImpactTriggered)
	{
		bImpactTriggered = true;

		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.Instigator = Character;
			CueParams.Location = Character ? Character->GetActorLocation() : FVector::ZeroVector;
			ASC->ExecuteGameplayCue(DDRTags::Cue_Slam, CueParams);
		}
	}

	// Montage sem Loop e End ja tocou: encerra no pouso.
	if (bEndSectionStarted)
	{
		return;
	}

	if (const ACharacter* C = Character)
	{
		if (USkeletalMeshComponent* Mesh = C->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (!AnimInstance->Montage_IsPlaying(SlamMontage))
				{
					EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
				}
			}
		}
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlamProximityTimerHandle);
	}

	if (UDDRCombatComponent* Combat = GetDDRCombatComponent())
	{
		if (Combat->IsSlamAirPinActive())
		{
			Combat->ReleaseSlamAirPinForLanding(PostSlamFallVelocity);
		}
		Combat->UnregisterSlamAbility();
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(DDRTags::State_Combat_SlamFall);
	}

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
