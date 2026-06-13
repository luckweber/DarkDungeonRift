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
#include "GE_DDRPoiseDamage.h"
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
	PoiseEffectClass = UGE_DDRPoiseDamage::StaticClass();
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

	SanitizeGroundLocomotionAfterAir();

	if (UAbilitySystemComponent* OwnerASC = GetOwnerASC())
	{
		if (const UDDRAttributeSet* AttrSet = OwnerASC->GetSet<UDDRAttributeSet>())
		{
			const_cast<UDDRAttributeSet*>(AttrSet)->TickPoiseRegen(DeltaTime);
		}
	}
}

void UDDRCombatComponent::SanitizeGroundLocomotionAfterAir()
{
	if (bInAirCombat || bSlamAirPinActive)
	{
		return;
	}

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr;
	if (!OwnerChar || !MoveComp || !MoveComp->IsMovingOnGround())
	{
		return;
	}

	if (bAirInputLocked)
	{
		UE_LOG(LogDDR, Warning, TEXT("[AIR] sanitize: input ainda travado no chao — liberando t=%.2f"),
			GetWorld()->GetTimeSeconds());
		UnlockAirHorizontalInput();
	}

	if (UDDRCharacterMovementComponent* DDRMove = Cast<UDDRCharacterMovementComponent>(MoveComp))
	{
		if (DDRMove->IsLocomotionInputBlocked())
		{
			DDRMove->SetLocomotionInputBlocked(false);
		}
	}

	if (MoveComp->MovementMode == MOVE_Flying)
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	if (!MoveComp->bOrientRotationToMovement)
	{
		MoveComp->bOrientRotationToMovement = true;
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

void UDDRCombatComponent::UnlockAirHorizontalInput(const bool bForce)
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
	// Safety net: se o slam morreu com o pin/hold ainda travando input, libera.
	if (!bInAirCombat)
	{
		UnlockAirHorizontalInput();
	}
}

void UDDRCombatComponent::BeginSlamAirPin()
{
	// Chamado por GA_AirSlam::TryJumpToEndSection (proximidade do chão / hitbox), NÃO pelo
	// ANS_DDRHitbox::NotifyBegin — pin cedo congela antes da velocity -3500 pousar (audit S-08).
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
			if (MoveComp->IsMovingOnGround())
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

	AActor* Target = GetPrimaryJuggleTarget();
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
		ReleaseSlamAirPinForLanding(Params.SlamFollowFallVelocity);
		UE_LOG(LogDDR, Log, TEXT("[SLAM] FollowToGround velZ=%.0f t=%.2f"),
			Params.SlamFollowFallVelocity, GetWorld()->GetTimeSeconds());
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
	if (!OwnerActor || !GetPrimaryJuggleTarget())
	{
		return;
	}

	ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(GetPrimaryJuggleTarget());
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

	// Re-sincroniza alvos após fixar o Z do player (audit A-01).
	PruneInvalidJuggleTargets();
	for (const TWeakObjectPtr<AActor>& WeakTarget : ActiveJuggleTargets)
	{
		if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(WeakTarget.Get()))
		{
			if (TargetChar->IsAirborne())
			{
				TargetChar->OverrideAirborneTargetZ(AirAnchorZ + JuggleTargetHeightAbovePlayer);
			}
		}
	}
}

namespace DDRMeleeHit
{
	/** Direção "de onde veio o golpe" para flinch 4-way (doc 63).
	 *  Atacante na frente → quadrante do ATACANTE (nao arco lateral da lamina).
	 *  Pass-through / atacante atras → socket inicio do sweep ou -SweepDir. */
	FVector ComputeHitFromDirection(
		const AActor* InstigatorActor,
		const AActor* VictimActor,
		const FVector& SweepStart,
		const FVector& SweepEnd,
		const FHitResult& Hit)
	{
		if (!VictimActor)
		{
			return FVector::ZeroVector;
		}

		const FVector VictimLoc = VictimActor->GetActorLocation();
		const FVector Forward = VictimActor->GetActorForwardVector().GetSafeNormal2D();

		FVector ToAttacker = FVector::ZeroVector;
		if (InstigatorActor)
		{
			ToAttacker = (InstigatorActor->GetActorLocation() - VictimLoc).GetSafeNormal2D();
		}

		const float AttackerFrontDot = FVector::DotProduct(Forward, ToAttacker);

		// Soft-lock / combo frontal: flinch pelo lado do atacante, nao pelo arco da espada.
		if (AttackerFrontDot > 0.f && !ToAttacker.IsNearlyZero())
		{
			return ToAttacker;
		}

		// Pass-through ou golpe por tras: origem do sweep (socket inicio) antes do corpo passar.
		const FVector ToSweepStart = (SweepStart - VictimLoc).GetSafeNormal2D();
		const float StartFrontDot = FVector::DotProduct(Forward, ToSweepStart);
		if (StartFrontDot > AttackerFrontDot && !ToSweepStart.IsNearlyZero())
		{
			return ToSweepStart;
		}

		const FVector SweepDir = (SweepEnd - SweepStart).GetSafeNormal2D();
		if (!SweepDir.IsNearlyZero())
		{
			return -SweepDir;
		}

		if (!ToAttacker.IsNearlyZero())
		{
			return ToAttacker;
		}

		return Hit.ImpactNormal.GetSafeNormal2D();
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

	// Lamina atravessa a capsula → 2 contatos (entrada/saida). Fica so o MAIS PROXIMO do
	// inicio do sweep (face de entrada) para dano + direcao do flinch.
	TMap<AActor*, FHitResult> BestHitPerActor;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			continue;
		}

		if (FHitResult* Existing = BestHitPerActor.Find(HitActor))
		{
			const FVector SweepDelta = End - Start;
			const float SweepLenSq = SweepDelta.SizeSquared2D();
			const float Along = SweepLenSq > KINDA_SMALL_NUMBER
				? FVector::DotProduct(Hit.ImpactPoint - Start, SweepDelta) / SweepLenSq
				: FVector::DistSquared(Start, Hit.ImpactPoint);
			const float ExistingAlong = SweepLenSq > KINDA_SMALL_NUMBER
				? FVector::DotProduct(Existing->ImpactPoint - Start, SweepDelta) / SweepLenSq
				: FVector::DistSquared(Start, Existing->ImpactPoint);
			if (Along < ExistingAlong)
			{
				*Existing = Hit;
			}
		}
		else
		{
			BestHitPerActor.Add(HitActor, Hit);
		}
	}

	for (const TPair<AActor*, FHitResult>& Pair : BestHitPerActor)
	{
		AActor* HitActor = Pair.Key;
		const FHitResult& Hit = Pair.Value;
		if (!CanHitActor(HitActor))
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
		const FVector HitFromDir = DDRMeleeHit::ComputeHitFromDirection(GetOwner(), HitActor, Start, End, Hit);
		ApplyDamageToTarget(HitActor, Params, HitFromDir);
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
				AddJuggleTarget(HitActor);
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
	if (bBufferedAttack)
	{
		BufferedAttackTimeRemaining = FMath::Max(BufferedAttackTimeRemaining, BufferSeconds);
		return;
	}

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

void UDDRCombatComponent::PruneInvalidJuggleTargets()
{
	ActiveJuggleTargets.RemoveAll([](const TWeakObjectPtr<AActor>& Entry)
	{
		return !Entry.IsValid();
	});
}

void UDDRCombatComponent::AddJuggleTarget(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	PruneInvalidJuggleTargets();

	for (const TWeakObjectPtr<AActor>& Existing : ActiveJuggleTargets)
	{
		if (Existing.Get() == TargetActor)
		{
			return;
		}
	}

	while (ActiveJuggleTargets.Num() >= MaxActiveJuggleTargets)
	{
		if (ADDRCharacterBase* Evicted = Cast<ADDRCharacterBase>(ActiveJuggleTargets[0].Get()))
		{
			if (Evicted->IsAirborne())
			{
				Evicted->EndAirborne(/*bSlammed=*/false);
			}
		}
		ActiveJuggleTargets.RemoveAt(0);
	}

	ActiveJuggleTargets.Add(TargetActor);
}

AActor* UDDRCombatComponent::GetPrimaryJuggleTarget() const
{
	for (int32 Index = ActiveJuggleTargets.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveJuggleTargets[Index].IsValid())
		{
			return ActiveJuggleTargets[Index].Get();
		}
	}
	return nullptr;
}

void UDDRCombatComponent::EndAllJuggleTargets(const bool bSlammed)
{
	PruneInvalidJuggleTargets();
	for (const TWeakObjectPtr<AActor>& WeakTarget : ActiveJuggleTargets)
	{
		if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(WeakTarget.Get()))
		{
			if (TargetChar->IsAirborne())
			{
				TargetChar->EndAirborne(bSlammed);
			}
		}
	}
	if (!bSlammed)
	{
		ActiveJuggleTargets.Reset();
	}
}

void UDDRCombatComponent::LaunchTarget(AActor* TargetActor)
{
	if (!CanLaunchActor(TargetActor))
	{
		UE_LOG(LogDDR, Log, TEXT("[LAUNCH] bloqueado — poise nao quebrou em %s"), *GetNameSafe(TargetActor));
		return;
	}

	if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(TargetActor))
	{
		// Altura inicial do alvo: LaunchRiseHeight (tune no BP para combinar com o RM do clip
		// Attack_Up_Floor_To_Air). Co-altitude fina acontece em EnterAirCombat, após o player
		// terminar o pulo — nunca puxa o player para baixo.
		TargetChar->StartAirborne(LaunchRiseHeight, TargetAirborneHoldSeconds);
		AddJuggleTarget(TargetActor);
		bLaunchedTargetThisSwing = true;
		LockAirHorizontalInput();

		// AUDITORIA — proibido neste ponto (ver timeline em DDRCombatComponent.h):
		// · RefreshAirHold / AirAnchorZ / pin de Z no tick (MaintainLaunchWindowAltitude)
		// · ExitAirCombat ou OnPlayerAirHoldExpired especial para "janela de launch"
		// Motivo: a montage do launcher ainda está em RM; o hold do player só começa em
		// EnterAirCombat (fim da montage). Timer cedo = dummy+player caem juntos ~1.1s.

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
	// Único lugar que liga bInAirCombat e ancora o Z do juggle (chamado por GA_Launcher
	// OnMontageCompleted quando DidLaunchTargetThisSwing). Whiff no launcher nunca chega aqui.
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
	bLaunchedTargetThisSwing = false;
	// Ancora na altitude do PULO do player (fim do RM do launcher) — não no inimigo.
	AirAnchorZ = OwnerChar->GetActorLocation().Z;
	bHasAirAnchor = true;

	// Alinha os alvos à co-altitude depois que o player subiu (doc 16 §2).
	PruneInvalidJuggleTargets();
	for (const TWeakObjectPtr<AActor>& WeakTarget : ActiveJuggleTargets)
	{
		if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(WeakTarget.Get()))
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

	// Hold do player so comeca apos o RM do launcher (nao no hit).
	GetWorld()->GetTimerManager().ClearTimer(AirHoldTimerHandle);
	RefreshAirHold();
}

void UDDRCombatComponent::RefreshAirHold()
{
	// Guard intencional: NÃO aceitar bLaunchedTargetThisSwing — o hit do launcher não conta
	// como "no ar" para o timer do player (ver LaunchTarget / regressão audit 62 A-02).
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
	// Early-return é esperado se R durante a montage do launcher (tag InAir do hit, mas
	// EnterAirCombat ainda não rodou). GA_AirSlam aplica a queda com velocity própria nesse caso.
	if (!bInAirCombat)
	{
		UE_LOG(LogDDR, Warning, TEXT("[SLAM] ExitAirCombat(bSlam=%d) IGNORADO — bInAirCombat ja era false (player nao estava no hold aereo)"),
			bSlam ? 1 : 0);
		return;
	}

	UE_LOG(LogDDR, Log, TEXT("[SLAM] ExitAirCombat bSlam=%d juggleTargets=%d t=%.2f"),
		bSlam ? 1 : 0, ActiveJuggleTargets.Num(), GetWorld()->GetTimeSeconds());

	// Slam reivindica os alvos: estende o hold pra ainda estarem no ar quando o impacto chegar.
	if (bSlam)
	{
		PruneInvalidJuggleTargets();
		if (ActiveJuggleTargets.Num() == 0)
		{
			UE_LOG(LogDDR, Warning, TEXT("[SLAM] sem juggleTarget valido no inicio do slam (nada para segurar no ar)"));
		}
		for (const TWeakObjectPtr<AActor>& WeakTarget : ActiveJuggleTargets)
		{
			if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(WeakTarget.Get()))
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
	}

	bInAirCombat = false;
	bHasAirAnchor = false;
	bAirCarryActive = false;
	if (!bSlam)
	{
		UE_LOG(LogDDR, Log, TEXT("[AIR] auto-drop: derrubando alvos junto com o player t=%.2f"),
			GetWorld()->GetTimeSeconds());
		EndAllJuggleTargets(/*bSlammed=*/false);
	}
	GetWorld()->GetTimerManager().ClearTimer(AirHoldTimerHandle);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		// Zera o count (nao decrementa) — garante que a tag SAI mesmo se algum caminho
		// futuro adicionar de novo; o gate do juggle e binario, nao empilhavel.
		ASC->SetLooseGameplayTagCount(DDRTags::State_Combat_InAir, 0);
	}

	UnlockAirHorizontalInput(/*bForce=*/true);

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (UCharacterMovementComponent* MoveComp = OwnerChar ? OwnerChar->GetCharacterMovement() : nullptr)
	{
		if (bSlam)
		{
			MoveComp->SetMovementMode(MOVE_Falling);
			MoveComp->Velocity = FVector(0.f, 0.f, -3500.f);
		}
		else if (MoveComp->IsMovingOnGround())
		{
			MoveComp->SetMovementMode(MOVE_Walking);
		}
		else
		{
			MoveComp->SetMovementMode(MOVE_Falling);
		}
	}
}

void UDDRCombatComponent::OnPlayerAirHoldExpired()
{
	// Sem ramo especial para bLaunchedTargetThisSwing — timeout só com bInAirCombat ativo.
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

	// Juggle ativo: prioriza alvos no ar (mais recente primeiro).
	if (bPreferAirborne)
	{
		for (int32 Index = ActiveJuggleTargets.Num() - 1; Index >= 0; --Index)
		{
			AActor* JuggleTarget = ActiveJuggleTargets[Index].Get();
			if (JuggleTarget && CanHitActor(JuggleTarget) && IsWithinVerticalGap(JuggleTarget))
			{
				const FVector To = JuggleTarget->GetActorLocation() - OwnerLoc;
				if (To.Size2D() <= SoftLockRadius)
				{
					return JuggleTarget;
				}
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

bool UDDRCombatComponent::SharesFactionWithOwner(AActor* OtherActor) const
{
	const UAbilitySystemComponent* SourceASC = GetOwnerASC();
	const UAbilitySystemComponent* TargetASC = OtherActor
		? OtherActor->FindComponentByClass<UAbilitySystemComponent>()
		: nullptr;

	if (!SourceASC || !TargetASC)
	{
		return false;
	}

	const bool bSourceEnemy = SourceASC->HasMatchingGameplayTag(DDRTags::Faction_Enemy);
	const bool bTargetEnemy = TargetASC->HasMatchingGameplayTag(DDRTags::Faction_Enemy);
	return bSourceEnemy && bTargetEnemy;
}

bool UDDRCombatComponent::CanLaunchActor(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		return true;
	}

	const float PoiseMax = TargetASC->GetNumericAttribute(UDDRAttributeSet::GetPoiseMaxAttribute());
	if (PoiseMax <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	if (TargetASC->HasMatchingGameplayTag(DDRTags::State_Combat_Stagger))
	{
		return true;
	}

	if (TargetASC->HasMatchingGameplayTag(DDRTags::Faction_Boss_NoLaunch))
	{
		return false;
	}

	const float Poise = TargetASC->GetNumericAttribute(UDDRAttributeSet::GetPoiseAttribute());
	return Poise <= KINDA_SMALL_NUMBER;
}

bool UDDRCombatComponent::CanHitActor(AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return false;
	}

	if (SharesFactionWithOwner(OtherActor))
	{
		return false;
	}

	// Caido (ragdoll OU knockdown animado) = janela de respiro — sem juggle/hit ate levantar.
	if (const ADDRCharacterBase* CharBase = Cast<ADDRCharacterBase>(OtherActor))
	{
		if (CharBase->IsRagdolled() || CharBase->IsKnockedDown())
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

void UDDRCombatComponent::ApplyDamageToTarget(AActor* TargetActor, const FDDRMeleeSweepParams& Params, const FVector& HitFromDirection2D)
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

	if (Params.PoiseDamage > 0.f)
	{
		ApplyPoiseToTarget(TargetActor, Params.PoiseDamage);
	}

	if (UWorld* World = GetWorld())
	{
		if (UDDRHitStopSubsystem* HitStop = World->GetSubsystem<UDDRHitStopSubsystem>())
		{
			HitStop->RequestHitStop(Params.HitStopFrames);
		}
	}

	// Reacao do alvo (doc 63): flinch 4-way por severidade; no ar usa o flinch aereo
	// (vende o juggle hit a hit). Launch/slam contam como heavy.
	if (ADDRCharacterBase* TargetChar = Cast<ADDRCharacterBase>(TargetActor))
	{
		const bool bHeavy = Params.bLaunchTargets || Params.bSlamDownTargets || Params.bHeavyHitReaction;
		TargetChar->PlayHitReaction(GetOwner(), bHeavy, HitFromDirection2D);
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

void UDDRCombatComponent::ApplyPoiseToTarget(AActor* TargetActor, const float PoiseDamage)
{
	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	UAbilitySystemComponent* TargetASC = TargetActor ? TargetActor->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
	if (!SourceASC || !TargetASC || !PoiseEffectClass || PoiseDamage <= 0.f)
	{
		return;
	}

	const float TargetPoiseMax = TargetASC->GetNumericAttribute(UDDRAttributeSet::GetPoiseMaxAttribute());
	if (TargetPoiseMax <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	Context.AddInstigator(GetOwner(), GetOwner());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(PoiseEffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DDRTags::Data_PoiseDamage, PoiseDamage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
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
