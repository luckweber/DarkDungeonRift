// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterBase.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "TimerManager.h"

ADDRCharacterBase::ADDRCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDDRCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AbilitySystemComponent = CreateDefaultSubobject<UDDRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UDDRAttributeSet>(TEXT("AttributeSet"));
	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());

	CombatComponent = CreateDefaultSubobject<UDDRCombatComponent>(TEXT("CombatComponent"));

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	MotionWarpingComponent->bSearchForWindowsInAnimsWithinMontages = true;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	// Tick só liga durante o juggle (StartAirborne) — barato no resto do tempo.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ADDRCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRagdolled)
	{
		TickRagdollFollow();
		return;
	}

	if (bGuidedSlamFall)
	{
		TickGuidedSlamFall(DeltaSeconds);
		return;
	}

	if (!bAirborneActive)
	{
		return;
	}

	FVector Location = GetActorLocation();

	if (bAirborneFollowEnabled && AirborneFollowAttacker.IsValid())
	{
		const AActor* Attacker = AirborneFollowAttacker.Get();
		const FVector AttackerLoc = Attacker->GetActorLocation();
		const FVector Fwd = Attacker->GetActorForwardVector();
		const FVector DesiredXY = AttackerLoc + Fwd * AirborneFollowForwardOffset;
		Location.X = FMath::FInterpTo(Location.X, DesiredXY.X, DeltaSeconds, AirborneInterpSpeed);
		Location.Y = FMath::FInterpTo(Location.Y, DesiredXY.Y, DeltaSeconds, AirborneInterpSpeed);
	}

	// Altura DIRIGIDA (doc 16 §2): interp suave até o Z alvo; o hold timer decide a queda.
	Location.Z = FMath::FInterpTo(Location.Z, AirborneTargetZ, DeltaSeconds, AirborneInterpSpeed);
	SetActorLocation(Location, /*bSweep=*/true);
}

void ADDRCharacterBase::SetAirborneFollow(AActor* Attacker, const float ForwardOffset, const bool bEnable)
{
	bAirborneFollowEnabled = bEnable && Attacker != nullptr;
	AirborneFollowAttacker = Attacker;
	AirborneFollowForwardOffset = ForwardOffset;
}

void ADDRCharacterBase::StartAirborne(float RiseHeight, float HoldSeconds)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Flying);
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->StopMovementImmediately();
	}

	if (!bAirborneActive)
	{
		AirborneHitCount = 0;
		AirborneTargetZ = GetActorLocation().Z + RiseHeight;
	}
	else
	{
		AirborneTargetZ += RiseHeight * 0.25f; // relançar no ar dá só um nudge
	}

	bAirborneActive = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	SetActorTickEnabled(true);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(DDRTags::State_Combat_Airborne);
	}

	RestartAirborneHoldTimer(HoldSeconds);

	UE_LOG(LogDDR, Log, TEXT("[AIRBORNE] %s StartAirborne rise=%.0f hold=%.2f targetZ=%.0f t=%.2f"),
		*GetName(), RiseHeight, HoldSeconds, AirborneTargetZ, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::ApplyAirPop(float PopHeight, float HoldSeconds)
{
	if (!bAirborneActive)
	{
		return;
	}

	++AirborneHitCount;
	AirborneTargetZ += PopHeight;
	RestartAirborneHoldTimer(HoldSeconds);
}

void ADDRCharacterBase::SetAirborneTargetZ(const float NewTargetZ, const float HoldSeconds)
{
	if (!bAirborneActive)
	{
		return;
	}

	++AirborneHitCount;
	AirborneTargetZ = NewTargetZ;
	RestartAirborneHoldTimer(HoldSeconds);

	UE_LOG(LogDDR, Log, TEXT("[AIRBORNE] %s SetTargetZ=%.0f hits=%d hold=%.2f t=%.2f"),
		*GetName(), NewTargetZ, AirborneHitCount, HoldSeconds, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::OverrideAirborneTargetZ(const float NewTargetZ)
{
	if (!bAirborneActive)
	{
		return;
	}

	AirborneTargetZ = NewTargetZ;
}

void ADDRCharacterBase::ExtendAirborneHold(const float HoldSeconds)
{
	if (!bAirborneActive)
	{
		UE_LOG(LogDDR, Warning, TEXT("[AIRBORNE] %s ExtendAirborneHold IGNORADO (nao esta airborne) t=%.2f"),
			*GetName(), GetWorld()->GetTimeSeconds());
		return;
	}

	RestartAirborneHoldTimer(HoldSeconds);

	UE_LOG(LogDDR, Log, TEXT("[AIRBORNE] %s hold ESTENDIDO %.2fs (slam) t=%.2f"),
		*GetName(), HoldSeconds, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::EndAirborne(bool bSlammed)
{
	if (!bAirborneActive)
	{
		UE_LOG(LogDDR, Warning, TEXT("[AIRBORNE] %s EndAirborne IGNORADO (ja nao esta airborne) slammed=%d t=%.2f"),
			*GetName(), bSlammed ? 1 : 0, GetWorld()->GetTimeSeconds());
		return;
	}

	const float SlamFallSpeed = GetSlamFallSpeed();

	UE_LOG(LogDDR, Log, TEXT("[AIRBORNE] %s EndAirborne slammed=%d z=%.0f vel=%.0f t=%.2f"),
		*GetName(), bSlammed ? 1 : 0, GetActorLocation().Z,
		bSlammed ? -SlamFallSpeed : 0.f, GetWorld()->GetTimeSeconds());

	bAirborneActive = false;
	AirborneHitCount = 0;
	bAirborneFollowEnabled = false;
	AirborneFollowAttacker.Reset();
	GetWorldTimerManager().ClearTimer(AirborneHoldTimerHandle);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(DDRTags::State_Combat_Airborne);
	}

	// Slam: queda guiada com sweep ate o chao; ragdoll so no POUSO (nao do ar — tunelava).
	if (bSlammed)
	{
		BeginSlamKnockdown(SlamFallSpeed);
		return;
	}

	SetActorTickEnabled(false);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Falling);
	}
}

void ADDRCharacterBase::BeginSlamKnockdown(const float SlamFallSpeed)
{
	bGuidedSlamFall = true;
	// Knockdown ANIMADO e o canonico (doc 63): a capsula dirige a queda (guided fall) e a
	// MONTAGE da a pose (Fall_Start -> Fall_Loop). Ragdoll vira fallback (sem montage/anim).
	const bool bAnimated = StartAnimatedKnockdownFall();
	bPendingRagdollOnSlamLand = !bAnimated && bRagdollOnSlammed;
	GuidedSlamFallStartTime = GetWorld()->GetTimeSeconds();
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Falling);
		MoveComp->Velocity = FVector::ZeroVector;
	}

	SetActorTickEnabled(true);

	UE_LOG(LogDDR, Log, TEXT("[SLAM] %s knockdown guiada ON%s velZ=-%.0f z=%.0f t=%.2f"),
		*GetName(),
		bPendingRagdollOnSlamLand ? TEXT(" -> ragdoll no pouso") : TEXT(""),
		SlamFallSpeed, GetActorLocation().Z, GetWorld()->GetTimeSeconds());
}

float ADDRCharacterBase::GetSlamFallSpeed() const
{
	return FMath::Min(FMath::Abs(SlammedFallVelocity), RagdollMaxInitialSpeed);
}

bool ADDRCharacterBase::TraceFloorBelow(const FVector& QueryLoc, FHitResult& OutHit) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DDRFloorTrace), false, this);
	const FVector Start = QueryLoc + FVector(0.f, 0.f, 50.f);
	const FVector End = QueryLoc - FVector(0.f, 0.f, 2000.f);

	static const ECollisionChannel FloorChannels[] = { ECC_WorldStatic, ECC_Visibility };
	for (const ECollisionChannel Channel : FloorChannels)
	{
		if (World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params)
			&& OutHit.bBlockingHit
			&& OutHit.ImpactNormal.Z > 0.5f)
		{
			return true;
		}
	}

	return false;
}

bool ADDRCharacterBase::SweepCapsuleToFloor(const FVector& From, const FVector& To, FHitResult& OutHit) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DDRGuidedSlamFall), false, this);
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius(),
		Capsule->GetScaledCapsuleHalfHeight());

	static const ECollisionChannel SweepChannels[] = { ECC_WorldStatic, ECC_Visibility, ECC_WorldDynamic };
	for (const ECollisionChannel Channel : SweepChannels)
	{
		if (World->SweepSingleByChannel(OutHit, From, To, FQuat::Identity, Channel, CapsuleShape, Params)
			&& OutHit.bBlockingHit
			&& OutHit.ImpactNormal.Z > 0.5f)
		{
			return true;
		}
	}

	return false;
}

void ADDRCharacterBase::FinishGuidedSlamFall()
{
	const bool bWantRagdoll = bPendingRagdollOnSlamLand;
	bGuidedSlamFall = false;
	bPendingRagdollOnSlamLand = false;

	// Knockdown animado: pousa -> seção de impacto (Fall_End) -> deitado (Ground) -> getup.
	if (bKnockedDown)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->SetMovementMode(MOVE_Walking);
		}
		SetActorTickEnabled(false);
		OnKnockdownLanded();
		return;
	}

	if (bWantRagdoll && bRagdollOnSlammed)
	{
		const FVector ImpactVel(0.f, 0.f, -SlamRagdollImpactSpeed);
		if (StartRagdoll(ImpactVel))
		{
			UE_LOG(LogDDR, Log, TEXT("[SLAM] %s knockdown POUSO -> ragdoll impacto z=%.0f t=%.2f"),
				*GetName(), GetActorLocation().Z, GetWorld()->GetTimeSeconds());
			return;
		}

		UE_LOG(LogDDR, Warning, TEXT("[SLAM] %s ragdoll falhou no pouso — fica em pe t=%.2f"),
			*GetName(), GetWorld()->GetTimeSeconds());
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	SetActorTickEnabled(false);

	UE_LOG(LogDDR, Log, TEXT("[SLAM] %s knockdown POUSO z=%.0f t=%.2f"),
		*GetName(), GetActorLocation().Z, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::TickGuidedSlamFall(float DeltaSeconds)
{
	if (bRagdolled)
	{
		bGuidedSlamFall = false;
		SetActorTickEnabled(false);
		return;
	}

	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(DDRTags::State_Dead))
	{
		FinishGuidedSlamFall();
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (World->GetTimeSeconds() - GuidedSlamFallStartTime > GuidedSlamFallTimeoutSeconds)
		{
			UE_LOG(LogDDR, Warning, TEXT("[SLAM] %s queda guiada TIMEOUT — forçando pouso t=%.2f"),
				*GetName(), World->GetTimeSeconds());
			FinishGuidedSlamFall();
			return;
		}
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		FinishGuidedSlamFall();
		return;
	}

	const FVector Loc = GetActorLocation();
	const float FallStep = GetSlamFallSpeed() * DeltaSeconds;
	const FVector Next = Loc - FVector(0.f, 0.f, FallStep);

	FHitResult Hit;
	if (SweepCapsuleToFloor(Loc, Next, Hit))
	{
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		SetActorLocation(
			FVector(Hit.Location.X, Hit.Location.Y, Hit.ImpactPoint.Z + HalfHeight),
			false, nullptr, ETeleportType::TeleportPhysics);
		FinishGuidedSlamFall();
		return;
	}

	FHitResult SweepHit;
	if (SetActorLocation(Next, true, &SweepHit) && SweepHit.bBlockingHit && SweepHit.ImpactNormal.Z > 0.5f)
	{
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		SetActorLocation(
			FVector(SweepHit.Location.X, SweepHit.Location.Y, SweepHit.ImpactPoint.Z + HalfHeight),
			false, nullptr, ETeleportType::TeleportPhysics);
		FinishGuidedSlamFall();
	}
}

void ADDRCharacterBase::TickRagdollFollow()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	FVector BoneLoc = MeshComp->GetBoneLocation(RagdollFollowBone);
	if (BoneLoc.IsNearlyZero())
	{
		return;
	}

	FHitResult Floor;
	if (TraceFloorBelow(BoneLoc, Floor))
	{
		const float MinPelvisZ = Floor.ImpactPoint.Z + RagdollPelvisGroundClearance;
		if (BoneLoc.Z < MinPelvisZ)
		{
			BoneLoc.Z = MinPelvisZ;

			const FVector Vel = MeshComp->GetPhysicsLinearVelocity(RagdollFollowBone);
			MeshComp->SetPhysicsLinearVelocity(
				FVector(Vel.X, Vel.Y, FMath::Max(0.f, Vel.Z)),
				false,
				RagdollFollowBone);

			if (Vel.SizeSquared() < 250000.f)
			{
				MeshComp->SetAllPhysicsLinearVelocity(FVector::ZeroVector, false);
			}
		}
	}

	SetActorLocation(BoneLoc, false, nullptr, ETeleportType::TeleportPhysics);
}

bool ADDRCharacterBase::StartRagdoll(const FVector& InitialVelocity)
{
	if (bRagdolled)
	{
		RecoverFromRagdoll();
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !MeshComp->GetPhysicsAsset())
	{
		UE_LOG(LogDDR, Warning, TEXT("[RAGDOLL] %s SEM Physics Asset na skeletal mesh — fallback: queda de capsula. Assigne um PA (SK_Mannequin tem)."),
			*GetName());
		return false;
	}

	bRagdolled = true;
	RagdollStartActorLocation = GetActorLocation();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// A mesh assume a colisao; a capsula sai do caminho (nao empurra o corpo caido).
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Cap na velocity (corpo a -4500 = 45 m/s TUNELAVA o chao fino) + CCD na queda.
	const FVector ClampedVel = InitialVelocity.GetClampedToMaxSize(RagdollMaxInitialSpeed);

	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllUseCCD(true);
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetAllPhysicsLinearVelocity(ClampedVel, false);
	MeshComp->WakeAllRigidBodies();

	// Tick liga pra capsula seguir o pelvis (sombra/lock-on acompanham o corpo).
	SetActorTickEnabled(true);

	if (RagdollRecoverSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			RagdollRecoverTimerHandle,
			this,
			&ADDRCharacterBase::OnRagdollRecoverExpired,
			RagdollRecoverSeconds,
			false);
	}

	UE_LOG(LogDDR, Log, TEXT("[RAGDOLL] %s ON vel=(%.0f, %.0f, %.0f) [cap %.0f] recover=%.1fs t=%.2f"),
		*GetName(), ClampedVel.X, ClampedVel.Y, ClampedVel.Z, RagdollMaxInitialSpeed,
		RagdollRecoverSeconds, GetWorld()->GetTimeSeconds());

	return true;
}

void ADDRCharacterBase::RecoverFromRagdoll()
{
	if (!bRagdolled)
	{
		return;
	}

	bRagdolled = false;
	GetWorldTimerManager().ClearTimer(RagdollRecoverTimerHandle);

	USkeletalMeshComponent* MeshComp = GetMesh();

	// Capsula levanta ONDE o corpo parou: chao sob o pelvis. Trace DE CIMA pra baixo —
	// se o corpo tunelou pra baixo do piso, ainda achamos o TOPO do piso (nao o void).
	const float HalfHeight = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 88.f;
	FVector BodyLoc = (MeshComp && !MeshComp->GetBoneLocation(RagdollFollowBone).IsNearlyZero())
		? MeshComp->GetBoneLocation(RagdollFollowBone)
		: GetActorLocation();

	FHitResult Floor;
	FVector StandLoc = BodyLoc;
	if (TraceFloorBelow(BodyLoc, Floor))
	{
		StandLoc.Z = Floor.ImpactPoint.Z + HalfHeight;
	}
	else
	{
		// Corpo no void (tunelou tudo): volta pra onde o knockdown comecou.
		StandLoc = RagdollStartActorLocation;
		UE_LOG(LogDDR, Warning, TEXT("[RAGDOLL] %s recover sem chao no trace — voltando ao inicio do knockdown"),
			*GetName());
	}
	SetActorLocation(StandLoc, false, nullptr, ETeleportType::TeleportPhysics);

	if (MeshComp)
	{
		MeshComp->SetAllBodiesSimulatePhysics(false);
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetAllUseCCD(false);
		MeshComp->SetCollisionProfileName(DefaultMeshCollisionProfile);
		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		MeshComp->SetRelativeTransform(DefaultMeshRelativeTransform);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// Dummy fica em pe; inimigos M3 trocam isto por anim de getup (doc 51, P1).
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	SetActorTickEnabled(false);

	UE_LOG(LogDDR, Log, TEXT("[RAGDOLL] %s OFF (recover) loc=(%.0f, %.0f, %.0f) t=%.2f"),
		*GetName(), StandLoc.X, StandLoc.Y, StandLoc.Z, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::OnRagdollRecoverExpired()
{
	RecoverFromRagdoll();
}

// ===== Knockdown animado + hit reactions (doc 63) =====

namespace DDRKnockdownSections
{
	static const FName FallStart(TEXT("Fall_Start"));
	static const FName FallLoop(TEXT("Fall_Loop"));
	static const FName FallEnd(TEXT("Fall_End"));
	static const FName Ground(TEXT("Ground"));
	static const FName GetUp(TEXT("GetUp"));
}

bool ADDRCharacterBase::StartAnimatedKnockdownFall()
{
	if (!KnockdownMontage)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogDDR, Warning, TEXT("[KNOCKDOWN] %s sem AnimInstance — fallback ragdoll/capsula"), *GetName());
		return false;
	}

	const float PlayResult = AnimInstance->Montage_Play(KnockdownMontage, 1.f,
		EMontagePlayReturnType::MontageLength, 0.f, /*bStopAllMontages=*/true);
	if (PlayResult <= 0.f)
	{
		UE_LOG(LogDDR, Warning, TEXT("[KNOCKDOWN] %s Montage_Play falhou — fallback"), *GetName());
		return false;
	}

	if (KnockdownMontage->GetSectionIndex(DDRKnockdownSections::FallStart) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(DDRKnockdownSections::FallStart, KnockdownMontage);
	}
	if (KnockdownMontage->GetSectionIndex(DDRKnockdownSections::FallLoop) != INDEX_NONE)
	{
		AnimInstance->Montage_SetNextSection(DDRKnockdownSections::FallStart, DDRKnockdownSections::FallLoop, KnockdownMontage);
		AnimInstance->Montage_SetNextSection(DDRKnockdownSections::FallLoop, DDRKnockdownSections::FallLoop, KnockdownMontage);
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ADDRCharacterBase::OnKnockdownMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, KnockdownMontage);

	bKnockedDown = true;

	// IA pausada o knockdown inteiro: o decorator do BT le Airborne OU Stagger (doc 61 §4.1).
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(DDRTags::State_Combat_Stagger);
	}

	UE_LOG(LogDDR, Log, TEXT("[KNOCKDOWN] %s queda ANIMADA ON t=%.2f"),
		*GetName(), GetWorld()->GetTimeSeconds());
	return true;
}

void ADDRCharacterBase::OnKnockdownLanded()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(KnockdownMontage))
	{
		// Montage morreu no caminho (interrompida): encerra limpo.
		FinishKnockdown();
		return;
	}

	if (KnockdownMontage->GetSectionIndex(DDRKnockdownSections::FallEnd) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(DDRKnockdownSections::FallEnd, KnockdownMontage);

		// Impacto -> deitado: Ground self-loopa ate o timer do getup.
		if (KnockdownMontage->GetSectionIndex(DDRKnockdownSections::Ground) != INDEX_NONE)
		{
			AnimInstance->Montage_SetNextSection(DDRKnockdownSections::FallEnd, DDRKnockdownSections::Ground, KnockdownMontage);
			AnimInstance->Montage_SetNextSection(DDRKnockdownSections::Ground, DDRKnockdownSections::Ground, KnockdownMontage);
		}
	}

	GetWorldTimerManager().SetTimer(
		KnockdownGroundTimerHandle,
		this,
		&ADDRCharacterBase::OnKnockdownGroundExpired,
		FMath::Max(KnockdownGroundSeconds, 0.05f),
		false);

	UE_LOG(LogDDR, Log, TEXT("[KNOCKDOWN] %s POUSO -> Fall_End -> Ground (%.1fs ate getup) t=%.2f"),
		*GetName(), KnockdownGroundSeconds, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::OnKnockdownGroundExpired()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance && AnimInstance->Montage_IsPlaying(KnockdownMontage)
		&& KnockdownMontage->GetSectionIndex(DDRKnockdownSections::GetUp) != INDEX_NONE)
	{
		// GetUp NAO tem next section -> a montage termina sozinha -> OnKnockdownMontageEnded.
		AnimInstance->Montage_JumpToSection(DDRKnockdownSections::GetUp, KnockdownMontage);
		UE_LOG(LogDDR, Log, TEXT("[KNOCKDOWN] %s GetUp t=%.2f"), *GetName(), GetWorld()->GetTimeSeconds());
		return;
	}

	FinishKnockdown();
}

void ADDRCharacterBase::OnKnockdownMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != KnockdownMontage)
	{
		return;
	}

	UE_LOG(LogDDR, Log, TEXT("[KNOCKDOWN] %s montage fim (interrompida=%d) t=%.2f"),
		*GetName(), bInterrupted ? 1 : 0, GetWorld()->GetTimeSeconds());
	FinishKnockdown();
}

void ADDRCharacterBase::FinishKnockdown()
{
	if (!bKnockedDown)
	{
		return;
	}

	bKnockedDown = false;
	GetWorldTimerManager().ClearTimer(KnockdownGroundTimerHandle);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(DDRTags::State_Combat_Stagger);
	}

	UE_LOG(LogDDR, Log, TEXT("[KNOCKDOWN] %s LEVANTOU (IA retoma) t=%.2f"),
		*GetName(), GetWorld()->GetTimeSeconds());
}

FName ADDRCharacterBase::ComputeHitReactionSection(const FVector& HitFromDirection2D, const AActor* InstigatorFallback) const
{
	static const FName SectionF(TEXT("F"));
	static const FName SectionB(TEXT("B"));
	static const FName SectionL(TEXT("L"));
	static const FName SectionR(TEXT("R"));

	// De onde veio o golpe, relativo ao MEU facing: impacto na frente = flinch F.
	FVector HitFrom = HitFromDirection2D.GetSafeNormal2D();
	if (HitFrom.IsNearlyZero() && InstigatorFallback)
	{
		HitFrom = (InstigatorFallback->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	}
	if (HitFrom.IsNearlyZero())
	{
		return SectionF;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		return SectionF;
	}

	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(
		FVector::CrossProduct(Forward, HitFrom).Z,
		FVector::DotProduct(Forward, HitFrom)));

	if (AngleDeg >= -45.f && AngleDeg <= 45.f)  return SectionF;
	if (AngleDeg > 45.f && AngleDeg <= 135.f)   return SectionR;
	if (AngleDeg < -45.f && AngleDeg >= -135.f) return SectionL;
	return SectionB;
}

void ADDRCharacterBase::PlayHitReaction(const AActor* InstigatorActor, const bool bHeavyHit, const FVector HitFromDirection2D)
{
	// Estados que ja SAO a reacao (ou maiores que ela): nao sobrepoe.
	if (bRagdolled || bKnockedDown || bGuidedSlamFall)
	{
		return;
	}

	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(DDRTags::State_Dead))
	{
		return;
	}

	// Flinch leve nao interrompe o proprio ataque (player OFF por default; inimigo ON —
	// trash interrompivel e design, doc 32; hyperarmor protege os fortes em P1).
	if (!bHeavyHit && !bLightHitReactionWhileAttacking
		&& AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(DDRTags::State_Combat_Attacking))
	{
		return;
	}

	UAnimMontage* Montage = nullptr;
	if (bAirborneActive)
	{
		Montage = AirHitReactionMontage;
	}
	else
	{
		Montage = bHeavyHit && HitReactionHeavyMontage ? HitReactionHeavyMontage : HitReactionMontage;
	}

	if (!Montage)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	const FName Section = ComputeHitReactionSection(HitFromDirection2D, InstigatorActor);
	if (AnimInstance->Montage_Play(Montage, 1.f) > 0.f
		&& Montage->GetSectionIndex(Section) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(Section, Montage);
	}

	UE_LOG(LogDDR, Log, TEXT("[HIT-REACT] %s %s secao=%s air=%d from=(%.2f,%.2f)"),
		*GetName(), bHeavyHit ? TEXT("HEAVY") : TEXT("light"), *Section.ToString(), bAirborneActive ? 1 : 0,
		HitFromDirection2D.X, HitFromDirection2D.Y);
}

void ADDRCharacterBase::RestartAirborneHoldTimer(float HoldSeconds)
{
	GetWorldTimerManager().SetTimer(
		AirborneHoldTimerHandle,
		this,
		&ADDRCharacterBase::OnAirborneHoldExpired,
		FMath::Max(HoldSeconds, 0.05f),
		false);
}

void ADDRCharacterBase::OnAirborneHoldExpired()
{
	UE_LOG(LogDDR, Log, TEXT("[AIRBORNE] %s hold EXPIROU (queda natural) t=%.2f"),
		*GetName(), GetWorld()->GetTimeSeconds());
	EndAirborne(/*bSlammed=*/false);
}

void ADDRCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// O SetupAttachment do construtor usa o default C++; re-anexa se o BP trocou o socket.
	if (WeaponMesh && GetMesh() && WeaponMesh->GetAttachSocketName() != WeaponSocketName)
	{
		WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, WeaponSocketName);
	}
}

UAbilitySystemComponent* ADDRCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UDDRCharacterMovementComponent* ADDRCharacterBase::GetDDRMovement() const
{
	return Cast<UDDRCharacterMovementComponent>(GetCharacterMovement());
}

void ADDRCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilitySystem();

	// Snapshot pro recover do ragdoll restaurar exatamente o que o BP configurou.
	if (const USkeletalMeshComponent* MeshComp = GetMesh())
	{
		DefaultMeshRelativeTransform = MeshComp->GetRelativeTransform();
		DefaultMeshCollisionProfile = MeshComp->GetCollisionProfileName();
	}
}

void ADDRCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void ADDRCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void ADDRCharacterBase::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent || bAbilitySystemInitialized)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	GrantStartupAbilities();
	bAbilitySystemInitialized = true;
}

void ADDRCharacterBase::GrantStartupAbilities()
{
	if (!AbilitySystemComponent || StartupAbilities.Num() == 0)
	{
		return;
	}

	for (const FDDRCStartupAbility& Grant : StartupAbilities)
	{
		if (!Grant.Ability)
		{
			continue;
		}

		const int32 InputID = Grant.InputID == EDDRAbilityInputID::None
			? INDEX_NONE
			: static_cast<int32>(Grant.InputID);

		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Grant.Ability, 1, InputID, this));
	}
}
