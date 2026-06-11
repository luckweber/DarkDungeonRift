// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterBase.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "Components/StaticMeshComponent.h"
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

	UE_LOG(LogDDR, Log, TEXT("[AIRBORNE] %s EndAirborne slammed=%d z=%.0f vel=%.0f t=%.2f"),
		*GetName(), bSlammed ? 1 : 0, GetActorLocation().Z,
		bSlammed ? SlammedFallVelocity : 0.f, GetWorld()->GetTimeSeconds());

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

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Falling);
		if (bSlammed)
		{
			MoveComp->Velocity = FVector(0.f, 0.f, SlammedFallVelocity);
		}
	}
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
