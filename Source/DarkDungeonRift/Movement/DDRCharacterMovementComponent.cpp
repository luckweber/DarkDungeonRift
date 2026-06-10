// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterMovementComponent.h"

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
	GravityScale = 1.f;
}

void UDDRCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (DashCooldownRemaining > 0.f)
	{
		DashCooldownRemaining = FMath::Max(0.f, DashCooldownRemaining - DeltaTime);
	}

	if (bIsDashing)
	{
		TickDash(DeltaTime);
	}
	else
	{
		UpdateGaitFromInput();
	}

	UpdateLocomotionState();

	if (CVarDDRLocomotionDebug.GetValueOnGameThread() > 0 && GetOwner())
	{
		const FVector Location = GetOwner()->GetActorLocation() + FVector(0.f, 0.f, 120.f);
		const FString DebugText = FString::Printf(
			TEXT("Gait=%s Speed=%.0f Sprint=%d Dash=%d CD=%.2f"),
			*UEnum::GetValueAsString(CurrentGait),
			Velocity.Size2D(),
			bWantsSprint ? 1 : 0,
			bIsDashing ? 1 : 0,
			DashCooldownRemaining);
		DrawDebugString(GetWorld(), Location, DebugText, nullptr, FColor::Cyan, 0.f, true);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

float UDDRCharacterMovementComponent::GetMaxSpeed() const
{
	if (bIsDashing)
	{
		return DashDistance / FMath::Max(DashDuration, KINDA_SMALL_NUMBER);
	}

	switch (CurrentGait)
	{
	case EDDRGait::Walk: return WalkSpeed;
	case EDDRGait::Run: return RunSpeed;
	case EDDRGait::Sprint: return SprintSpeed;
	default: return RunSpeed;
	}
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

bool UDDRCharacterMovementComponent::TryDash(const FVector& WorldDirection)
{
	if (bIsDashing || DashCooldownRemaining > 0.f)
	{
		return false;
	}

	const FVector Direction = GetDashDirection(WorldDirection);
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	bIsDashing = true;
	DashTimeRemaining = DashDuration;
	DashDirection = Direction;
	DashCooldownRemaining = DashCooldown;
	Velocity = DashDirection * GetMaxSpeed();
	SetMovementMode(MOVE_Flying);

	return true;
}

void UDDRCharacterMovementComponent::UpdateGaitFromInput()
{
	const bool bHasMovementInput = !Acceleration.IsNearlyZero() && Acceleration.Size2D() > 1.f;
	const EDDRGait DesiredGait = (bWantsSprint && bHasMovementInput) ? EDDRGait::Sprint : EDDRGait::Run;
	if (DesiredGait != CurrentGait)
	{
		SetGait(DesiredGait);
	}
}

void UDDRCharacterMovementComponent::UpdateLocomotionState()
{
	LocomotionState.Velocity = Velocity;
	LocomotionState.Acceleration = Acceleration;
	LocomotionState.Speed = Velocity.Size2D();
	LocomotionState.bIsMoving = LocomotionState.Speed > 10.f;
	LocomotionState.bIsFalling = IsFalling();
	LocomotionState.Gait = CurrentGait;
	LocomotionState.bIsDashing = bIsDashing;
	LocomotionState.bWantsToStop = LocomotionState.bIsMoving && Acceleration.IsNearlyZero();

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

void UDDRCharacterMovementComponent::TickDash(float DeltaTime)
{
	DashTimeRemaining -= DeltaTime;
	Velocity = DashDirection * GetMaxSpeed();

	if (DashTimeRemaining <= 0.f)
	{
		EndDash();
	}
}

void UDDRCharacterMovementComponent::EndDash()
{
	bIsDashing = false;
	DashTimeRemaining = 0.f;
	Velocity = DashDirection * RunSpeed * 0.5f;
	SetMovementMode(MOVE_Walking);
	SetGait(EDDRGait::Run);
}

FVector UDDRCharacterMovementComponent::GetDashDirection(const FVector& InputDirection) const
{
	FVector Direction = InputDirection;
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero())
	{
		if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			Direction = OwnerCharacter->GetActorForwardVector();
		}
	}

	return Direction.GetSafeNormal2D();
}
