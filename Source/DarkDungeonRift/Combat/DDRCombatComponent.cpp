// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DDRAttributeSet.h"
#include "DDRCharacterBase.h"
#include "DDRGameplayTags.h"
#include "DDRMotionWarpLibrary.h"
#include "MotionWarpingComponent.h"
#include "DDRHitStopSubsystem.h"
#include "DDRLog.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GA_AirSlam.h"
#include "GE_DDRDamage.h"
#include "DDRCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarCombatDebug(
	TEXT("ddr.CombatDebug"),
	0,
	TEXT("1 = log + debug draw (blade sweep volume, impact points, combo windows)."),
	ECVF_Default);

UDDRCombatComponent::UDDRCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Depois do CMC/root motion — corrige Z do juggle no mesmo frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	DamageEffectClass = UGE_DDRDamage::StaticClass();
}

void UDDRCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBufferedAttack)
	{
		BufferedAttackTimeRemaining -= DeltaTime;
		if (BufferedAttackTimeRemaining <= 0.f)
		{
			ClearBufferedAttack();
		}
	}

	if (bInAirCombat)
	{
		MaintainAirAltitude();
	}

	if (bAirCarryActive)
	{
		SyncJuggleTargetFollow();
	}

	if (bSlamAirPinActive)
	{
		MaintainSlamAirPin();
	}

	if (bAirInputLocked)
	{
		MaintainAirInputLock();
	}
}

void UDDRCombatComponent::ApplyLauncherAirTuning(
	const float InLaunchRiseHeight,
	const float InJuggleTargetHeightAbovePlayer)
{
	LaunchRiseHeight = InLaunchRiseHeight;
	JuggleTargetHeightAbovePlayer = InJuggleTargetHeightAbovePlayer;
}

void UDDRCombatComponent::ApplyAirAttackJuggleTuning(
	const float InJuggleTargetHeightAbovePlayer,
	const float InAirPopVerticalNudgeScale)
{
	JuggleTargetHeightAbovePlayer = InJuggleTargetHeightAbovePlayer;
	AirPopVerticalNudgeScale = InAirPopVerticalNudgeScale;
}

void UDDRCombatComponent::NotifyDashEnded()
{
	if (const UWorld* World = GetWorld())
	{
		LastDashEndTimeSeconds = World->GetTimeSeconds();
	}
}

float UDDRCombatComponent::GetTimeSinceDashEnded() const
{
	const UWorld* World = GetWorld();
	if (!World || LastDashEndTimeSeconds < 0.f)
	{
		return FLT_MAX;
	}
	return World->GetTimeSeconds() - LastDashEndTimeSeconds;
}

void UDDRCombatComponent::LockAirHorizontalInput()
{
	if (bAirInputLocked)
	{
		return;
	}

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (!MoveComp)
	{
		return;
	}

	bAirInputLocked = true;
	SavedMaxFlySpeed = MoveComp->MaxFlySpeed > KINDA_SMALL_NUMBER ? MoveComp->MaxFlySpeed : 600.f;
	MoveComp->MaxFlySpeed = 0.f;

	if (!bOrientLockedForAir)
	{
		bSavedOrientToMovement = MoveComp->bOrientRotationToMovement;
		MoveComp->bOrientRotationToMovement = false;
		bOrientLockedForAir = true;
	}

	if (UDDRCharacterMovementComponent* DDRMove = Cast<UDDRCharacterMovementComponent>(MoveComp))
	{
		DDRMove->SetLocomotionInputBlocked(true);
	}

	MoveComp->StopMovementImmediately();

	UE_LOG(LogDDR, Log, TEXT("[AIR] input horizontal TRAVADO (hold aereo/pin) t=%.2f"),
		GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::UnlockAirHorizontalInput()
{
	if (!bAirInputLocked)
	{
		return;
	}

	bAirInputLocked = false;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (MoveComp)
	{
		MoveComp->MaxFlySpeed = SavedMaxFlySpeed > KINDA_SMALL_NUMBER ? SavedMaxFlySpeed : 600.f;

		if (bOrientLockedForAir)
		{
			MoveComp->bOrientRotationToMovement = bSavedOrientToMovement;
			bOrientLockedForAir = false;
		}

		if (UDDRCharacterMovementComponent* DDRMove = Cast<UDDRCharacterMovementComponent>(MoveComp))
		{
			DDRMove->SetLocomotionInputBlocked(false);
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[AIR] input horizontal LIBERADO t=%.2f"), GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::MaintainAirInputLock()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (!OwnerChar || !MoveComp)
	{
		return;
	}

	FVector Vel = MoveComp->Velocity;
	Vel.X = 0.f;
	Vel.Y = 0.f;
	MoveComp->Velocity = Vel;
}

void UDDRCombatComponent::RegisterSlamAbility(UGA_AirSlam* SlamAbility)
{
	ActiveSlamAbility = SlamAbility;
	bSlamEndJumpedThisSwing = false;
}

void UDDRCombatComponent::UnregisterSlamAbility()
{
	ActiveSlamAbility.Reset();
	bSlamAirPinActive = false;
	bSlamEndJumpedThisSwing = false;
	bSlamPinSweepParamsSet = false;
	ActiveJuggleTarget.Reset();
	// Safety net: se o slam morreu com o pin/hold ainda travando input, libera.
	if (!bInAirCombat)
	{
		UnlockAirHorizontalInput();
	}
}

void UDDRCombatComponent::BeginSlamAirPin()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return;
	}

	if (bSlamAirPinActive)
	{
		return;
	}

	bSlamAirPinActive = true;
	SlamPinZ = OwnerChar->GetActorLocation().Z;

	if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Flying);
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->StopMovementImmediately();
	}

	// Pin tambem e Flying — trava o WASD (cobre o slam que cortou o launcher,
	// onde EnterAirCombat nunca rodou).
	LockAirHorizontalInput();

	UE_LOG(LogDDR, Log, TEXT("[SLAM] PinInAir ON z=%.0f t=%.2f"), SlamPinZ, GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::EndSlamAirPin(const FDDRMeleeSweepParams& Params)
{
	if (!bSlamAirPinActive)
	{
		return;
	}

	// Secao End: notify curto — manter congelado ate a montage acabar (nao relancar -3500).
	if (const UGA_AirSlam* Slam = ActiveSlamAbility.Get())
	{
		if (Slam->IsEndSectionStarted())
		{
			UE_LOG(LogDDR, Log, TEXT("[SLAM] PinInAir notify fim — mantendo no ar (End em andamento) t=%.2f"),
				GetWorld()->GetTimeSeconds());
			return;
		}
	}

	bSlamAirPinActive = false;
	UnlockAirHorizontalInput();

	const float FallVel = Params.bResumeFallAfterSlamPin ? Params.SlamFollowFallVelocity : 0.f;
	if (UGA_AirSlam* Slam = ActiveSlamAbility.Get())
	{
		Slam->ResumeFallAfterSlamPin(FallVel, Params.bResumeFallAfterSlamPin);
	}
	else if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_Falling);
			if (Params.bResumeFallAfterSlamPin)
			{
				FVector Vel = MoveComp->Velocity;
				Vel.Z = FallVel;
				MoveComp->Velocity = Vel;
			}
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] PinInAir OFF resumeFall=%d velZ=%.0f t=%.2f"),
		Params.bResumeFallAfterSlamPin ? 1 : 0, FallVel, GetWorld()->GetTimeSeconds());

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
		{
			if (MoveComp->MovementMode == MOVE_Flying && !MoveComp->IsFalling())
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}
		}
	}
}

void UDDRCombatComponent::ReleaseSlamAirPinForLanding(const float PostFallVelocityZ)
{
	if (!bSlamAirPinActive)
	{
		return;
	}

	bSlamAirPinActive = false;
	bSlamPinSweepParamsSet = false;
	UnlockAirHorizontalInput();

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// QUEDA NATURAL (AAA): nada de SetActorLocation pro chao ("teleporte") — solta em
	// Falling com velZ inicial (0 = gravidade pura, ~0.7s de 300cm) e o AnimBP assume:
	// IsFalling -> Fall Loop da locomocao -> pouso com anim de land (doc 13 / 58 §1.3).
	MoveComp->SetMovementMode(MOVE_Falling);
	FVector Vel = FVector::ZeroVector;
	Vel.Z = FMath::Min(PostFallVelocityZ, 0.f);
	MoveComp->Velocity = Vel;

	UE_LOG(LogDDR, Log, TEXT("[SLAM] PinInAir RELEASE -> queda natural velZ=%.0f de z=%.0f t=%.2f (Fall Loop ate o pouso)"),
		Vel.Z, OwnerChar->GetActorLocation().Z, GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::SnapSlamEndToJuggleTarget()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return;
	}

	AActor* Target = ActiveJuggleTarget.Get();
	if (!Target)
	{
		Target = FindSoftLockTarget(/*bPreferAirborne=*/true);
	}

	const ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(Target);
	if (!TargetChar || !TargetChar->IsAirborne())
	{
		return;
	}

	FVector Loc = OwnerChar->GetActorLocation();
	const float TargetZ = TargetChar->GetActorLocation().Z;
	Loc.Z = TargetZ;
	OwnerChar->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);

	UE_LOG(LogDDR, Log, TEXT("[SLAM] snap co-altitude alvoZ=%.0f playerZ=%.0f t=%.2f"),
		TargetZ, Loc.Z, GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::SetSlamPinSweepParams(const FDDRMeleeSweepParams& Params)
{
	SlamPinSweepParams = Params;
	bSlamPinSweepParamsSet = true;
}

void UDDRCombatComponent::MaintainSlamAirPin()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (!OwnerChar || !MoveComp)
	{
		return;
	}

	if (MoveComp->MovementMode != MOVE_Flying)
	{
		MoveComp->SetMovementMode(MOVE_Flying);
	}

	MoveComp->Velocity = FVector::ZeroVector;

	const FVector Loc = OwnerChar->GetActorLocation();
	if (!FMath::IsNearlyEqual(Loc.Z, SlamPinZ, 0.5f))
	{
		FVector Pinned = Loc;
		Pinned.Z = SlamPinZ;
		OwnerChar->SetActorLocation(Pinned, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (bSlamPinSweepParamsSet)
	{
		if (const UGA_AirSlam* Slam = ActiveSlamAbility.Get())
		{
			if (Slam->IsEndSectionStarted())
			{
				PerformMeleeSweep(SlamPinSweepParams);
			}
		}
	}
}

void UDDRCombatComponent::ApplySlamPlayerFollow(const FDDRMeleeSweepParams& Params, AActor* HitActor)
{
	if (Params.SlamPlayerFollow == EDDRSlamPlayerFollow::FollowToGround)
	{
		if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
			{
				bSlamAirPinActive = false;
				UnlockAirHorizontalInput();
				MoveComp->SetMovementMode(MOVE_Falling);
				FVector Vel = MoveComp->Velocity;
				Vel.Z = Params.SlamFollowFallVelocity;
				MoveComp->Velocity = Vel;
				UE_LOG(LogDDR, Log, TEXT("[SLAM] FollowToGround velZ=%.0f t=%.2f"),
					Params.SlamFollowFallVelocity, GetWorld()->GetTimeSeconds());
			}
		}
	}
}

void UDDRCombatComponent::SetAirCarryActive(const bool bActive, const float ForwardOffset)
{
	bAirCarryActive = bActive;
	AirCarryForwardOffset = ForwardOffset;
	if (!bActive)
	{
		SyncJuggleTargetFollow();
	}
}

void UDDRCombatComponent::SyncJuggleTargetFollow()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !ActiveJuggleTarget.IsValid())
	{
		return;
	}

	ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(ActiveJuggleTarget.Get());
	if (!TargetChar || !TargetChar->IsAirborne())
	{
		return;
	}

	TargetChar->SetAirborneFollow(OwnerActor, AirCarryForwardOffset, bAirCarryActive);
}

void UDDRCombatComponent::MaintainAirAltitude()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (!OwnerChar || !MoveComp || !bHasAirAnchor)
	{
		return;
	}

	if (MoveComp->MovementMode != MOVE_Flying)
	{
		MoveComp->SetMovementMode(MOVE_Flying);
	}

	FVector Velocity = MoveComp->Velocity;
	Velocity.Z = 0.f;
	MoveComp->Velocity = Velocity;

	const FVector Loc = OwnerChar->GetActorLocation();
	if (!FMath::IsNearlyEqual(Loc.Z, AirAnchorZ, 0.5f))
	{
		FVector Pinned = Loc;
		Pinned.Z = AirAnchorZ;
		OwnerChar->SetActorLocation(Pinned, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UDDRCombatComponent::PerformMeleeSweep(const FDDRMeleeSweepParams& Params)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Sweep da LAMINA (doc 18 par.2): entre os sockets weapon_start/weapon_end da arma.
	// Fallback (sem arma ou sem sockets): sweep frontal do ator — comportamento pre-M1.
	FVector Start;
	FVector End;
	bool bBladeSweep = false;

	// AoE do slam (doc 16 par.5): COLUNA vertical no dono — esfera varrida do ponto de
	// pouso para cima (SweepReach = altura). Esfera pura no chao NAO alcancava o alvo
	// juggleado ~300cm acima (gap vertical > raio) — o slam "errava" o proprio juggle.
	if (Params.bAoEAtOwner)
	{
		Start = OwnerActor->GetActorLocation();
		End = Start + FVector(0.f, 0.f, FMath::Max(Params.SweepReach, 0.f));
	}
	else if (const ADDRCharacterBase* OwnerChar = Cast<ADDRCharacterBase>(OwnerActor))
	{
		if (UStaticMeshComponent* Weapon = OwnerChar->GetWeaponMesh())
		{
			if (Weapon->GetStaticMesh()
				&& Weapon->DoesSocketExist(WeaponTraceSocketStart)
				&& Weapon->DoesSocketExist(WeaponTraceSocketEnd))
			{
				Start = Weapon->GetSocketLocation(WeaponTraceSocketStart);
				End = Weapon->GetSocketLocation(WeaponTraceSocketEnd);
				bBladeSweep = true;
			}
		}
	}

	if (!bBladeSweep && !Params.bAoEAtOwner)
	{
		Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		const FVector Forward = OwnerActor->GetActorForwardVector();
		End = Start + Forward * (Params.SweepForwardOffset + Params.SweepReach);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DDRMeleeSweep), false, OwnerActor);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(Params.SweepRadius),
		QueryParams);

	if (Params.bAoEAtOwner)
	{
		UE_LOG(LogDDR, Log, TEXT("[SLAM] AoE coluna r=%.0f z=%.0f ate %.0f | candidatos no sweep=%d"),
			Params.SweepRadius, Start.Z, End.Z + Params.SweepRadius, Hits.Num());
	}

	int32 NewHits = 0;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || !CanHitActor(HitActor))
		{
			// Gated: o sweep continuo do pin no End chamaria isto todo frame (ragdoll caido).
			if (Params.bAoEAtOwner && CVarCombatDebug.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogDDR, Log, TEXT("[SLAM] AoE ignorou %s (self/morto/ragdoll)"), *GetNameSafe(HitActor));
			}
			continue;
		}

		if (HitActorsThisSwing.Contains(HitActor))
		{
			continue;
		}

		HitActorsThisSwing.Add(HitActor);
		++NewHits;
		ApplyDamageToTarget(HitActor, Params);
		SendHitEvent(HitActor);

		if (Params.bLaunchTargets)
		{
			LaunchTarget(HitActor);
		}
		else if (Params.bAirPop)
		{
			AirPopTarget(HitActor);
			if (Params.bCarryAirborneTargets)
			{
				ActiveJuggleTarget = HitActor;
				SetAirCarryActive(true, Params.AirCarryForwardOffset);
			}
		}
		else if (Params.bSlamDownTargets)
		{
			ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(HitActor);
			const bool bTargetAirborne = TargetChar && TargetChar->IsAirborne();
			UE_LOG(LogDDR, Log, TEXT("[SLAM] hit %s | airborne=%d follow=%d jumpEnd=%d"),
				*GetNameSafe(HitActor), bTargetAirborne ? 1 : 0,
				static_cast<int32>(Params.SlamPlayerFollow), Params.bJumpToSlamEndSection ? 1 : 0);
			if (bTargetAirborne)
			{
				TargetChar->EndAirborne(/*bSlammed=*/true);
				ApplySlamPlayerFollow(Params, HitActor);
			}

			if (Params.bJumpToSlamEndSection && !bSlamEndJumpedThisSwing)
			{
				bSlamEndJumpedThisSwing = true;
				if (UGA_AirSlam* Slam = ActiveSlamAbility.Get())
				{
					Slam->NotifySlamHitboxJumpToEnd();
				}
			}
		}

		UE_LOG(LogDDR, Log, TEXT("[HIT] %s secao=%d dano=%.1f blade=%d t=%.2f"),
			*GetNameSafe(HitActor), Params.ComboSectionIndex, Params.BaseDamage,
			bBladeSweep ? 1 : 0, GetWorld()->GetTimeSeconds());

#if ENABLE_DRAW_DEBUG
		if (CVarCombatDebug.GetValueOnGameThread() > 0)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 12.f, 8, FColor::Yellow, false, 0.8f);
		}
#endif
	}

#if ENABLE_DRAW_DEBUG
	if (CVarCombatDebug.GetValueOnGameThread() > 0)
	{
		const FColor SweepColor = NewHits > 0 ? FColor::Red : FColor::Green;
		DrawDebugLine(GetWorld(), Start, End, SweepColor, false, 0.5f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), Start, Params.SweepRadius, 12, SweepColor, false, 0.5f);
		DrawDebugSphere(GetWorld(), End, Params.SweepRadius, 12, SweepColor, false, 0.5f);
	}
#endif
}

void UDDRCombatComponent::OpenComboWindow()
{
	bComboWindowOpen = true;
	UE_LOG(LogDDR, Log, TEXT("[ATK] janela ABRE (secao %d) t=%.2f"),
		ActiveComboSection, GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::CloseComboWindow()
{
	if (bComboWindowOpen)
	{
		UE_LOG(LogDDR, Log, TEXT("[ATK] janela FECHA (secao %d) t=%.2f"),
			ActiveComboSection, GetWorld()->GetTimeSeconds());
	}
	bComboWindowOpen = false;
}

void UDDRCombatComponent::BufferAttackInput(float BufferSeconds)
{
	bBufferedAttack = true;
	BufferedAttackTimeRemaining = BufferSeconds;
	UE_LOG(LogDDR, Log, TEXT("[ATK] buffer armado %.2fs t=%.2f"),
		BufferSeconds, GetWorld()->GetTimeSeconds());
}

void UDDRCombatComponent::ClearBufferedAttack()
{
	bBufferedAttack = false;
	BufferedAttackTimeRemaining = 0.f;
}

bool UDDRCombatComponent::HasBufferedAttack() const
{
	return bBufferedAttack && BufferedAttackTimeRemaining > 0.f;
}

void UDDRCombatComponent::ResetHitTracking()
{
	HitActorsThisSwing.Reset();
	bLaunchedTargetThisSwing = false;
}

void UDDRCombatComponent::LaunchTarget(AActor* TargetActor)
{
	if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(TargetActor))
	{
		// Altura inicial do alvo: LaunchRiseHeight (tune no BP para combinar com o RM do clip
		// Attack_Up_Floor_To_Air). Co-altitude fina acontece em EnterAirCombat, após o player
		// terminar o pulo — nunca puxa o player para baixo.
		TargetChar->StartAirborne(LaunchRiseHeight, TargetAirborneHoldSeconds);
		ActiveJuggleTarget = TargetActor;
		bLaunchedTargetThisSwing = true;
		LockAirHorizontalInput();

		// Gate de abilities: assim que alguem foi lancado, LMB vira AirAttack (nao espera
		// o fim da montage do launcher — senao AttackLight dispara no meio do uppercut).
		// SetLooseGameplayTagCount (nao Add): loose tags sao ref-counted; Add aqui + Add no
		// EnterAirCombat deixaria count 2 e o Remove unico do ExitAirCombat vazaria a tag.
		if (UAbilitySystemComponent* SourceASC = GetOwnerASC())
		{
			SourceASC->SetLooseGameplayTagCount(DDRTags::State_Combat_InAir, 1);

			FGameplayCueParameters CueParams;
			CueParams.Instigator = GetOwner();
			CueParams.Location = TargetActor->GetActorLocation();
			SourceASC->ExecuteGameplayCue(DDRTags::Cue_Launch, CueParams);
		}
	}
}

void UDDRCombatComponent::AirPopTarget(AActor* TargetActor)
{
	ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(TargetActor);
	if (!TargetChar || !TargetChar->IsAirborne())
	{
		return;
	}

	// Cap + decay (doc 16 par.3): cada hit re-flutua menos; no teto, o alvo cai.
	if (TargetChar->GetAirborneHitCount() >= MaxJuggleHits)
	{
		TargetChar->EndAirborne(/*bSlammed=*/false);
		return;
	}

	// Co-altitude: altura RELATIVA ao player, não stack infinito de +150cm por hit.
	const AActor* OwnerActor = GetOwner();
	const float RefZ = (bInAirCombat && bHasAirAnchor && OwnerActor)
		? AirAnchorZ
		: (OwnerActor ? OwnerActor->GetActorLocation().Z : TargetChar->GetActorLocation().Z);

	const float Nudge = AirPopHeightBase * AirPopVerticalNudgeScale
		* FMath::Pow(AirPopDecay, static_cast<float>(TargetChar->GetAirborneHitCount()));
	const float DesiredZ = RefZ + JuggleTargetHeightAbovePlayer + Nudge;
	TargetChar->SetAirborneTargetZ(DesiredZ, TargetAirborneHoldSeconds);
}

void UDDRCombatComponent::EnterAirCombat()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (!MoveComp)
	{
		return;
	}

	if (!bInAirCombat)
	{
		SavedMovementMode = static_cast<uint8>(MoveComp->MovementMode);
	}

	bInAirCombat = true;
	// Ancora na altitude do PULO do player (fim do RM do launcher) — não no inimigo.
	AirAnchorZ = OwnerChar->GetActorLocation().Z;
	bHasAirAnchor = true;

	// Alinha o alvo à co-altitude depois que o player subiu (doc 16 §2).
	if (ActiveJuggleTarget.IsValid())
	{
		if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(ActiveJuggleTarget.Get()))
		{
			if (TargetChar->IsAirborne())
			{
				TargetChar->OverrideAirborneTargetZ(AirAnchorZ + JuggleTargetHeightAbovePlayer);
			}
		}
	}
	MoveComp->SetMovementMode(MOVE_Flying);
	MoveComp->Velocity = FVector::ZeroVector;
	MoveComp->StopMovementImmediately();

	// Flying aceita WASD — sem o lock dava pra "andar no ar" durante o juggle.
	LockAirHorizontalInput();

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		// Idempotente — LaunchTarget ja pode ter posto a tag no hit (count fica 1, nunca 2).
		ASC->SetLooseGameplayTagCount(DDRTags::State_Combat_InAir, 1);
	}

	RefreshAirHold();
}

void UDDRCombatComponent::RefreshAirHold()
{
	if (!bInAirCombat)
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		AirHoldTimerHandle,
		this,
		&UDDRCombatComponent::OnPlayerAirHoldExpired,
		FMath::Max(PlayerAirHoldSeconds, 0.1f),
		false);
}

void UDDRCombatComponent::ExitAirCombat(bool bSlam)
{
	if (!bInAirCombat)
	{
		UE_LOG(LogDDR, Warning, TEXT("[SLAM] ExitAirCombat(bSlam=%d) IGNORADO — bInAirCombat ja era false (player nao estava no hold aereo)"),
			bSlam ? 1 : 0);
		return;
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] ExitAirCombat bSlam=%d juggleTarget=%s t=%.2f"),
		bSlam ? 1 : 0, *GetNameSafe(ActiveJuggleTarget.Get()), GetWorld()->GetTimeSeconds());

	// Slam reivindica o alvo: estende o hold pra ele AINDA estar no ar quando o impacto
	// chegar (sem isto o hold de 1.1s expirava na descida e o bSlamDownTargets no-opava
	// — o dummy "descia de leve" antes do slam conectar).
	if (bSlam && ActiveJuggleTarget.IsValid())
	{
		if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(ActiveJuggleTarget.Get()))
		{
			if (TargetChar->IsAirborne())
			{
				TargetChar->ExtendAirborneHold(SlamTargetHoldSeconds);
			}
			else
			{
				UE_LOG(LogDDR, Warning, TEXT("[SLAM] juggleTarget %s JA NAO esta airborne no inicio do slam (hold expirou cedo?)"),
					*GetNameSafe(TargetChar));
			}
		}
	}
	else if (bSlam)
	{
		UE_LOG(LogDDR, Warning, TEXT("[SLAM] sem juggleTarget valido no inicio do slam (nada para segurar no ar)"));
	}

	bInAirCombat = false;
	bHasAirAnchor = false;
	bAirCarryActive = false;
	if (!bSlam)
	{
		ActiveJuggleTarget.Reset();
	}
	GetWorld()->GetTimerManager().ClearTimer(AirHoldTimerHandle);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		// Zera o count (nao decrementa) — garante que a tag SAI mesmo se algum caminho
		// futuro adicionar de novo; o gate do juggle e binario, nao empilhavel.
		ASC->SetLooseGameplayTagCount(DDRTags::State_Combat_InAir, 0);
	}

	UnlockAirHorizontalInput();

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr)
	{
		MoveComp->SetMovementMode(MOVE_Falling);
		if (bSlam)
		{
			MoveComp->Velocity = FVector(0.f, 0.f, -3500.f);
		}
	}
}

void UDDRCombatComponent::OnPlayerAirHoldExpired()
{
	ExitAirCombat(/*bSlam=*/false);
}

AActor* UDDRCombatComponent::FindSoftLockTarget(bool bPreferAirborne) const
{
	if (!bSoftLockEnabled)
	{
		return nullptr;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	const FVector OwnerLoc = OwnerActor->GetActorLocation();

	auto IsWithinVerticalGap = [this, &OwnerLoc](const AActor* Candidate) -> bool
	{
		if (!Candidate)
		{
			return false;
		}
		return FMath::Abs(Candidate->GetActorLocation().Z - OwnerLoc.Z) <= SoftLockMaxVerticalGap;
	};

	// Juggle ativo: prioriza o alvo que já está no ar (se ainda na mesma faixa de altura).
	if (bPreferAirborne && ActiveJuggleTarget.IsValid() && CanHitActor(ActiveJuggleTarget.Get()))
	{
		if (IsWithinVerticalGap(ActiveJuggleTarget.Get()))
		{
			const FVector To = ActiveJuggleTarget->GetActorLocation() - OwnerLoc;
			if (To.Size2D() <= SoftLockRadius)
			{
				return ActiveJuggleTarget.Get();
			}
		}
	}

	// Direção da INTENÇÃO: input de movimento atual > facing.
	FVector PreferredDir = OwnerActor->GetActorForwardVector();
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		const FVector Input = OwnerPawn->GetLastMovementInputVector();
		if (!Input.IsNearlyZero())
		{
			PreferredDir = Input;
		}
	}
	PreferredDir = PreferredDir.GetSafeNormal2D();

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(SoftLockHalfAngleDegrees));

	AActor* BestInCone = nullptr;
	float BestScore = -FLT_MAX;
	AActor* NearestAny = nullptr;
	float NearestDist = FLT_MAX;

	for (TActorIterator<ADDRCharacterBase> It(GetWorld()); It; ++It)
	{
		ADDRCharacterBase* Candidate = *It;
		if (!Candidate || Candidate == OwnerActor || !CanHitActor(Candidate))
		{
			continue;
		}

		const FVector To = Candidate->GetActorLocation() - OwnerLoc;
		const float Dist = To.Size2D();
		if (Dist > SoftLockRadius)
		{
			continue;
		}

		if (!IsWithinVerticalGap(Candidate))
		{
			continue;
		}

		const float CosA = FVector::DotProduct(PreferredDir, To.GetSafeNormal2D());

		// Ângulo pesa mais que distância; alvo juggleado domina quando pedido.
		float Score = CosA * 200.f - Dist * 0.5f;
		if (bPreferAirborne && Candidate->IsAirborne())
		{
			Score += 1000.f;
		}

		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			NearestAny = Candidate;
		}

		if (CosA >= CosHalfAngle && Score > BestScore)
		{
			BestScore = Score;
			BestInCone = Candidate;
		}
	}

	// Juggle aéreo: fallback no mais próximo, mas só na frente (nunca costas).
	if (bPreferAirborne)
	{
		if (BestInCone)
		{
			return BestInCone;
		}
		if (NearestAny)
		{
			const FVector To = NearestAny->GetActorLocation() - OwnerLoc;
			const float CosA = FVector::DotProduct(PreferredDir, To.GetSafeNormal2D());
			if (CosA >= 0.f)
			{
				return NearestAny;
			}
		}
		return nullptr;
	}

	// Chão: só alvo no cone à frente.
	return BestInCone;
}

bool UDDRCombatComponent::IsTargetInAttackArc(const AActor* Target) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Target || !OwnerActor)
	{
		return false;
	}

	FVector PreferredDir = OwnerActor->GetActorForwardVector();
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		const FVector Input = OwnerPawn->GetLastMovementInputVector();
		if (!Input.IsNearlyZero())
		{
			PreferredDir = Input;
		}
	}
	PreferredDir = PreferredDir.GetSafeNormal2D();

	const FVector To = Target->GetActorLocation() - OwnerActor->GetActorLocation();
	const float CosA = FVector::DotProduct(PreferredDir, To.GetSafeNormal2D());
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(SoftLockHalfAngleDegrees));
	return CosA >= CosHalfAngle;
}

AActor* UDDRCombatComponent::FaceSoftLockTarget(bool bPreferAirborne)
{
	AActor* Target = FindSoftLockTarget(bPreferAirborne);
	AActor* OwnerActor = GetOwner();
	if (!Target || !OwnerActor || !IsTargetInAttackArc(Target))
	{
		return nullptr;
	}

	FVector To = Target->GetActorLocation() - OwnerActor->GetActorLocation();
	To.Z = 0.f;
	if (!To.IsNearlyZero())
	{
		FRotator NewRot = OwnerActor->GetActorRotation();
		NewRot.Yaw = To.Rotation().Yaw;
		OwnerActor->SetActorRotation(NewRot);
	}

#if ENABLE_DRAW_DEBUG
	if (CVarCombatDebug.GetValueOnGameThread() > 0)
	{
		DrawDebugLine(GetWorld(), OwnerActor->GetActorLocation(), Target->GetActorLocation(),
			FColor::Cyan, false, 0.4f, 0, 1.5f);
	}
#endif

	return Target;
}

AActor* UDDRCombatComponent::FaceAndSetupMotionWarp(const EDDRMotionWarpProfile Profile, const bool bPreferAirborne)
{
	AActor* Target = FindSoftLockTarget(bPreferAirborne);
	const bool bCanAssist = Target && IsTargetInAttackArc(Target);

	if (bCanAssist)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			FVector To = Target->GetActorLocation() - OwnerActor->GetActorLocation();
			To.Z = 0.f;
			if (!To.IsNearlyZero())
			{
				FRotator NewRot = OwnerActor->GetActorRotation();
				NewRot.Yaw = To.Rotation().Yaw;
				OwnerActor->SetActorRotation(NewRot);
			}
		}
	}

	if (!bMotionWarpEnabled || Profile == EDDRMotionWarpProfile::None || !bCanAssist)
	{
		return Target;
	}

	if (ADDRCharacterBase* OwnerChar = Cast<ADDRCharacterBase>(GetOwner()))
	{
		if (UMotionWarpingComponent* MW = OwnerChar->GetMotionWarpingComponent())
		{
			const bool bWarped = DDRMotionWarp::ApplyAttackWarp(
				MW,
				Target,
				Profile,
				IdealHitDistance,
				MaxWarpDistance,
				MaxWarpDistanceAir,
				MaxWarpDistanceLauncher);

			UE_LOG(LogDDR, Log, TEXT("[WARP] perfil=%d alvo=%s aplicado=%d%s"),
				static_cast<int32>(Profile), *GetNameSafe(Target), bWarped ? 1 : 0,
				(Target && !bWarped) ? TEXT(" (fora do cap — whiff honesto)") : TEXT(""));
		}
	}

	return Target;
}

void UDDRCombatComponent::ClearAttackMotionWarp()
{
	if (ADDRCharacterBase* OwnerChar = Cast<ADDRCharacterBase>(GetOwner()))
	{
		if (UMotionWarpingComponent* MW = OwnerChar->GetMotionWarpingComponent())
		{
			DDRMotionWarp::ClearAttackWarp(MW);
		}
	}
}

void UDDRCombatComponent::SetActiveComboSection(int32 SectionIndex)
{
	ActiveComboSection = SectionIndex;
}

float UDDRCombatComponent::GetHealthPercent() const
{
	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		const float Health = ASC->GetNumericAttribute(UDDRAttributeSet::GetHealthAttribute());
		const float MaxHealth = ASC->GetNumericAttribute(UDDRAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth > KINDA_SMALL_NUMBER)
		{
			return Health / MaxHealth;
		}
	}

	return 1.f;
}

bool UDDRCombatComponent::CanHitActor(AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return false;
	}

	// Caido (ragdoll) = janela de respiro do knockdown — sem juggle/hit ate levantar.
	if (const ADDRCharacterBase* CharBase = Cast<ADDRCharacterBase>(OtherActor))
	{
		if (CharBase->IsRagdolled())
		{
			return false;
		}
	}

	if (UAbilitySystemComponent* TargetASC = OtherActor->FindComponentByClass<UAbilitySystemComponent>())
	{
		if (TargetASC->HasMatchingGameplayTag(DDRTags::State_Dead))
		{
			return false;
		}
	}

	return true;
}

void UDDRCombatComponent::ApplyDamageToTarget(AActor* TargetActor, const FDDRMeleeSweepParams& Params)
{
	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	UAbilitySystemComponent* TargetASC = TargetActor ? TargetActor->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
	if (!SourceASC || !TargetASC || !DamageEffectClass)
	{
		return;
	}

	float Damage = Params.BaseDamage;
	if (const float AttackPower = SourceASC->GetNumericAttribute(UDDRAttributeSet::GetAttackPowerAttribute()))
	{
		Damage *= (1.f + AttackPower * DamageAttackPowerScale);
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	Context.AddInstigator(GetOwner(), GetOwner());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DDRTags::Data_Damage, Damage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (UWorld* World = GetWorld())
	{
		if (UDDRHitStopSubsystem* HitStop = World->GetSubsystem<UDDRHitStopSubsystem>())
		{
			HitStop->RequestHitStop(Params.HitStopFrames);
		}
	}

	if (SourceASC->IsValidLowLevel())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetOwner();
		CueParams.EffectCauser = GetOwner();
		CueParams.Location = TargetActor->GetActorLocation();
		SourceASC->ExecuteGameplayCue(DDRTags::Cue_Hit_Light, CueParams);
	}
}

void UDDRCombatComponent::SendHitEvent(AActor* HitActor) const
{
	FGameplayEventData EventData;
	EventData.Instigator = GetOwner();
	EventData.Target = HitActor;
	EventData.EventTag = DDRTags::Event_Combat_Hit;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), DDRTags::Event_Combat_Hit, EventData);
}

UAbilitySystemComponent* UDDRCombatComponent::GetOwnerASC() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
}
