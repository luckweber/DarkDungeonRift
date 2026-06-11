// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "DDRCharacterBase.h"
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
			TEXT("Gait=%s Speed=%.0f Sprint=%d Dash=%d Fall=%d"),
			*UEnum::GetValueAsString(CurrentGait),
			Velocity.Size2D(),
			bWantsSprint ? 1 : 0,
			LocomotionState.bIsDashing ? 1 : 0,
			LocomotionState.bIsFalling ? 1 : 0);
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

