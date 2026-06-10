// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DDRLocomotionTypes.h"
#include "DDRCharacterMovementComponent.generated.h"

UCLASS(ClassGroup = (DDR), meta = (BlueprintSpawnableComponent))
class DARKDUNGEONRIFT_API UDDRCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UDDRCharacterMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetMaxSpeed() const override;

	UFUNCTION(BlueprintCallable, Category = "DDR|Gait")
	void SetGait(EDDRGait NewGait);

	UFUNCTION(BlueprintCallable, Category = "DDR|Gait")
	EDDRGait GetCurrentGait() const { return CurrentGait; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Gait")
	void SetWantsSprint(bool bNewWantsSprint);

	UFUNCTION(BlueprintCallable, Category = "DDR|Movement")
	bool TryDash(const FVector& WorldDirection);

	UFUNCTION(BlueprintCallable, Category = "DDR|Movement")
	bool IsDashing() const { return bIsDashing; }

	UFUNCTION(BlueprintPure, Category = "DDR|Locomotion")
	const FDDRLocomotionState& GetLocomotionState() const { return LocomotionState; }

	UPROPERTY(EditAnywhere, Category = "DDR|Gait")
	float WalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Gait")
	float RunSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Gait")
	float SprintSpeed = 750.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Dash")
	float DashDistance = 550.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Dash")
	float DashDuration = 0.22f;

	UPROPERTY(EditAnywhere, Category = "DDR|Dash")
	float DashCooldown = 0.6f;

protected:
	void UpdateGaitFromInput();
	void UpdateLocomotionState();
	void TickDash(float DeltaTime);
	void EndDash();
	FVector GetDashDirection(const FVector& InputDirection) const;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Gait")
	EDDRGait CurrentGait = EDDRGait::Run;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Gait")
	bool bWantsSprint = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Movement")
	bool bIsDashing = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Movement")
	float DashTimeRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Movement")
	float DashCooldownRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Movement")
	FVector DashDirection = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Locomotion")
	FDDRLocomotionState LocomotionState;
};
