// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "DDRCharacterBase.h"
#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLocomotionTypes.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "DDRLog.h"

static TAutoConsoleVariable<int32> CVarDDRLocomotionDebug(
	TEXT("ddr.LocomotionDebug"),
	0,
	TEXT("Draw locomotion debug overlay. 0=off, 1=on"),
	ECVF_Cheat);

UDDRCharacterMovementComponent::UDDRCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	MaxWalkSpeed = RunSpeed;
	RotationRate = FRotator(0.f, 720.f, 0.f);
	JumpZVelocity = 700.f;
	AirControl = 0.35f;
	BrakingDecelerationWalking = 2000.f;
	BrakingDecelerationFalling = 1500.f;
	// Gravidade "de jogo" (doc 13 §2): >1 = queda com peso; a descida ainda
	// multiplica por FallingGravityMultiplier (GetGravityZ).
	GravityScale = 1.75f;
}

void UDDRCharacterMovementComponent::SetLocomotionInputBlocked(const bool bBlocked)
{
	bBlockLocomotionInput = bBlocked;
	if (bBlocked)
	{
		Acceleration = FVector::ZeroVector;
		FVector Vel = Velocity;
		Vel.X = 0.f;
		Vel.Y = 0.f;
		Velocity = Vel;
	}
}

void UDDRCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bBlockLocomotionInput)
	{
		Acceleration = FVector::ZeroVector;
		FVector Vel = Velocity;
		Vel.X = 0.f;
		Vel.Y = 0.f;
		Velocity = Vel;
	}

	UpdateGaitFromInput();
	UpdateLocomotionState();

	if (CVarDDRLocomotionDebug.GetValueOnGameThread() > 0 && GetOwner())
	{
		const FVector Location = GetOwner()->GetActorLocation() + FVector(0.f, 0.f, 120.f);
		const FString DebugText = FString::Printf(
			TEXT("Gait=%s Speed=%.0f Sprint=%d Dash=%d Fall=%d Jump=%s Air=%d/%d"),
			*UEnum::GetValueAsString(CurrentGait),
			Velocity.Size2D(),
			bWantsSprint ? 1 : 0,
			LocomotionState.bIsDashing ? 1 : 0,
			LocomotionState.bIsFalling ? 1 : 0,
			*UEnum::GetValueAsString(LocomotionState.JumpDirection),
			AirJumpCount, MaxAirJumps);
		DrawDebugString(GetWorld(), Location, DebugText, nullptr, FColor::Cyan, 0.f, true);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

float UDDRCharacterMovementComponent::GetMaxSpeed() const
{
	switch (CurrentGait)
	{
	case EDDRGait::Walk: return WalkSpeed;
	case EDDRGait::Run: return RunSpeed;
	case EDDRGait::Sprint: return SprintSpeed;
	default: return RunSpeed;
	}
}

float UDDRCharacterMovementComponent::GetGravityZ() const
{
	const float Gravity = Super::GetGravityZ(); // já inclui GravityScale

	// Gravidade assimétrica (doc 13 §3): sobe "leve", cai com peso.
	if (IsFalling() && Velocity.Z < 0.f)
	{
		return Gravity * FallingGravityMultiplier;
	}

	return Gravity;
}

void UDDRCharacterMovementComponent::SetGait(EDDRGait NewGait)
{
	CurrentGait = NewGait;
	MaxWalkSpeed = GetMaxSpeed();
}

void UDDRCharacterMovementComponent::SetWantsSprint(bool bNewWantsSprint)
{
	bWantsSprint = bNewWantsSprint;
}

void UDDRCharacterMovementComponent::OnMovementModeChanged(
	const EMovementMode PreviousMovementMode,
	const uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	// Flying tambem: o slam pin pode soltar direto Flying->Walking (EndSlamAirPin) —
	// sem isto AirJumpCount ficava >0 no chao e o pulo MORRIA ate cair de novo.
	if ((PreviousMovementMode == MOVE_Falling || PreviousMovementMode == MOVE_Flying) && IsMovingOnGround())
	{
		ResetAirJumpState();
	}
}

EDDRJumpDirection UDDRCharacterMovementComponent::ComputeJumpDirection(const ACharacter* Character) const
{
	if (!Character)
	{
		return EDDRJumpDirection::Neutral;
	}

	FVector InputDir = Character->GetLastMovementInputVector();
	InputDir.Z = 0.f;
	if (InputDir.SizeSquared() < FMath::Square(JumpDirectionInputDeadzone))
	{
		return EDDRJumpDirection::Neutral;
	}

	InputDir.Normalize();

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		return EDDRJumpDirection::Neutral;
	}

	const float CrossZ = FVector::CrossProduct(Forward, InputDir).Z;
	const float Dot = FVector::DotProduct(Forward, InputDir);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));

	if (AngleDeg >= -45.f && AngleDeg <= 45.f)
	{
		return EDDRJumpDirection::Forward;
	}
	if (AngleDeg > 45.f && AngleDeg <= 135.f)
	{
		return EDDRJumpDirection::Right;
	}
	if (AngleDeg < -45.f && AngleDeg >= -135.f)
	{
		return EDDRJumpDirection::Left;
	}

	return EDDRJumpDirection::Back;
}

void UDDRCharacterMovementComponent::ResetAirJumpState()
{
	AirJumpCount = 0;
	LocomotionState.AirJumpIndex = 0;
	LocomotionState.bIsDoubleJump = false;
	LocomotionState.bJumpFromAir = false;
	bDoubleJumpJustTriggered = false;
	// JumpDirection / LandDirection mantidos para o estado Land do AnimBP.
}

bool UDDRCharacterMovementComponent::TryCombatJump(ACharacter* Character)
{
	if (!Character || bBlockLocomotionInput)
	{
		return false;
	}

	if (const ADDRCharacterBase* OwnerChar = Cast<ADDRCharacterBase>(Character))
	{
		if (const UDDRCombatComponent* Combat = OwnerChar->GetDDRCombat())
		{
			if (Combat->IsAirHorizontalInputLocked())
			{
				return false;
			}
		}

		// Slam descendente (doc 59 §6): pulo BLOQUEADO — o double jump anularia a
		// velocity -3500 do mergulho (player subiria com a montage do slam rodando).
		if (const UAbilitySystemComponent* ASC = OwnerChar->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(DDRTags::State_Combat_SlamFall))
			{
				UE_LOG(LogDDR, Log, TEXT("[JUMP] bloqueado (SlamFall) t=%.2f"),
					GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
				return false;
			}
		}
	}

	const EDDRJumpDirection Dir = ComputeJumpDirection(Character);

	if (IsFalling())
	{
		if (AirJumpCount >= MaxAirJumps)
		{
			return false;
		}

		// Queda de BORDA (nunca pulou): o 1º pulo aereo consome tambem o slot do chao —
		// senao cair de plataforma daria DOIS impulsos aereos vs UM apos pulo normal.
		if (AirJumpCount == 0)
		{
			AirJumpCount = 1;
		}

		AirJumpCount++;
		LocomotionState.JumpDirection = Dir;
		LocomotionState.LandDirection = Dir;
		LocomotionState.AirJumpIndex = AirJumpCount;
		bDoubleJumpJustTriggered = true;       // pulso: transição Fall Loop → Jump
		LocomotionState.bJumpFromAir = true;   // persistente: conteúdo do estado usa Double anim

		FVector NewVel = Velocity;
		NewVel.Z = FMath::Max(NewVel.Z, 0.f) + DoubleJumpZVelocity;
		Velocity = NewVel;

		UE_LOG(LogDDR, Log, TEXT("[JUMP] double dir=%s zVel=%.0f air=%d/%d t=%.2f"),
			*UEnum::GetValueAsString(Dir), DoubleJumpZVelocity, AirJumpCount, MaxAirJumps,
			GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);

		return true;
	}

	if (!Character->CanJump() || AirJumpCount > 0)
	{
		return false;
	}

	AirJumpCount = 1;
	LocomotionState.JumpDirection = Dir;
	LocomotionState.LandDirection = Dir;
	LocomotionState.AirJumpIndex = 1;
	LocomotionState.bIsDoubleJump = false;
	bDoubleJumpJustTriggered = false;
	LocomotionState.bJumpFromAir = false;  // pulo do chão → Jump_Combat_Start_*

	Character->Jump();

	UE_LOG(LogDDR, Log, TEXT("[JUMP] ground dir=%s zVel=%.0f t=%.2f"),
		*UEnum::GetValueAsString(Dir), JumpZVelocity,
		GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);

	return true;
}

void UDDRCharacterMovementComponent::UpdateGaitFromInput()
{
	if (bBlockLocomotionInput)
	{
		return;
	}

	const bool bHasMovementInput = !Acceleration.IsNearlyZero() && Acceleration.Size2D() > 1.f;
	const EDDRGait DesiredGait = (bWantsSprint && bHasMovementInput) ? EDDRGait::Sprint : EDDRGait::Run;
	if (DesiredGait != CurrentGait)
	{
		SetGait(DesiredGait);
	}
}

void UDDRCharacterMovementComponent::UpdateLocomotionState()
{
	LocomotionState.bIsDoubleJump = bDoubleJumpJustTriggered;
	if (bDoubleJumpJustTriggered)
	{
		bDoubleJumpJustTriggered = false;
	}

	LocomotionState.Velocity = Velocity;
	LocomotionState.Acceleration = Acceleration;
	LocomotionState.Speed = Velocity.Size2D();
	LocomotionState.bIsMoving = LocomotionState.Speed > 10.f;
	LocomotionState.bIsFalling = IsFalling();
	LocomotionState.Gait = CurrentGait;
	LocomotionState.bWantsToStop = LocomotionState.bIsMoving && Acceleration.IsNearlyZero();

	// Dash é GAS (GA_Dash): o estado vem da tag concedida pelo GE_DDRDashIFrames.
	LocomotionState.bIsDashing = false;
	if (const ADDRCharacterBase* OwnerChar = Cast<ADDRCharacterBase>(GetOwner()))
	{
		if (const UAbilitySystemComponent* ASC = OwnerChar->GetAbilitySystemComponent())
		{
			LocomotionState.bIsDashing = ASC->HasMatchingGameplayTag(DDRTags::State_Movement_Dashing);
			LocomotionState.bIsCombatFalling = ASC->HasMatchingGameplayTag(DDRTags::State_Combat_SlamFall);
			LocomotionState.bIsAirCombat = ASC->HasMatchingGameplayTag(DDRTags::State_Combat_InAir);
		}
	}

	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
		const FVector Velocity2D = Velocity.GetSafeNormal2D();
		if (!Velocity2D.IsNearlyZero() && !Forward.IsNearlyZero())
		{
			LocomotionState.Direction = FMath::RadiansToDegrees(FMath::Atan2(
				FVector::CrossProduct(Forward, Velocity2D).Z,
				FVector::DotProduct(Forward, Velocity2D)));
		}
	}
}

