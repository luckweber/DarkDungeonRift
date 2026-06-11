// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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
#include "GE_DDRDamage.h"
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
			if (Params.bAoEAtOwner)
			{
				UE_LOG(LogDDR, Log, TEXT("[SLAM] AoE ignorou %s (self/morto/invalido)"), *GetNameSafe(HitActor));
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
			UE_LOG(LogDDR, Log, TEXT("[SLAM] AoE acertou %s | castBase=%d airborne=%d -> slamdown=%d"),
				*GetNameSafe(HitActor), TargetChar ? 1 : 0, bTargetAirborne ? 1 : 0, bTargetAirborne ? 1 : 0);
			if (bTargetAirborne)
			{
				TargetChar->EndAirborne(/*bSlammed=*/true);
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
	ActiveJuggleTarget.Reset();
	GetWorld()->GetTimerManager().ClearTimer(AirHoldTimerHandle);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		// Zera o count (nao decrementa) — garante que a tag SAI mesmo se algum caminho
		// futuro adicionar de novo; o gate do juggle e binario, nao empilhavel.
		ASC->SetLooseGameplayTagCount(DDRTags::State_Combat_InAir, 0);
	}

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

	// Fora do cone? Em topdown a intenção é "bate no que está perto" — fallback no mais próximo.
	return BestInCone ? BestInCone : NearestAny;
}

AActor* UDDRCombatComponent::FaceSoftLockTarget(bool bPreferAirborne)
{
	AActor* Target = FindSoftLockTarget(bPreferAirborne);
	AActor* OwnerActor = GetOwner();
	if (!Target || !OwnerActor)
	{
		return Target;
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
	AActor* Target = FaceSoftLockTarget(bPreferAirborne);

	if (!bMotionWarpEnabled || Profile == EDDRMotionWarpProfile::None)
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
