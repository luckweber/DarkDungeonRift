// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterBase.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
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
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearTimer(AirborneHoldTimerHandle);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(DDRTags::State_Combat_Airborne);
	}

	// Knockdown AAA: ragdoll fisico (cap de velocidade + CCD). Sem PA -> queda guiada com sweep
	// (nao usa -4500 na capsula — tunelava o chao em 1-2 frames).
	if (bSlammed && bRagdollOnSlammed && StartRagdoll(FVector(0.f, 0.f, -SlamFallSpeed)))
	{
		return;
	}

	if (bSlammed)
	{
		bGuidedSlamFall = true;
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_Falling);
			MoveComp->Velocity = FVector(0.f, 0.f, -SlamFallSpeed);
		}
		SetActorTickEnabled(true);
		UE_LOG(LogDDR, Log, TEXT("[SLAM] %s queda guiada ON velZ=-%.0f t=%.2f"),
			*GetName(), SlamFallSpeed, GetWorld()->GetTimeSeconds());
		return;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Falling);
	}
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

void ADDRCharacterBase::FinishGuidedSlamFall()
{
	bGuidedSlamFall = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	SetActorTickEnabled(false);

	UE_LOG(LogDDR, Log, TEXT("[SLAM] %s queda guiada POUSO z=%.0f t=%.2f"),
		*GetName(), GetActorLocation().Z, GetWorld()->GetTimeSeconds());
}

void ADDRCharacterBase::TickGuidedSlamFall(float DeltaSeconds)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		FinishGuidedSlamFall();
		return;
	}

	const FVector Loc = GetActorLocation();
	const float FallStep = GetSlamFallSpeed() * DeltaSeconds;
	const FVector Next = Loc - FVector(0.f, 0.f, FallStep);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DDRGuidedSlamFall), false, this);
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius(),
		Capsule->GetScaledCapsuleHalfHeight());

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(
		Hit, Loc, Next, FQuat::Identity, ECC_WorldStatic, CapsuleShape, Params)
		&& Hit.bBlockingHit
		&& Hit.ImpactNormal.Z > 0.5f)
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

			// Tunelou fundo demais: recover imediato em vez de ficar no void.
			if (BoneLoc.Z < Floor.ImpactPoint.Z + 25.f)
			{
				UE_LOG(LogDDR, Warning, TEXT("[RAGDOLL] %s pelvis abaixo do chao — recover antecipado t=%.2f"),
					*GetName(), GetWorld()->GetTimeSeconds());
				RecoverFromRagdoll();
				return;
			}
		}
	}

	SetActorLocation(BoneLoc, false, nullptr, ETeleportType::TeleportPhysics);
}

bool ADDRCharacterBase::StartRagdoll(const FVector& InitialVelocity)
{
	if (bRagdolled)
	{
		return true;
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
