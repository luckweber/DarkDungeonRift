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
	virtual float GetGravityZ() const override;

	UFUNCTION(BlueprintCallable, Category = "DDR|Gait")
	void SetGait(EDDRGait NewGait);

	UFUNCTION(BlueprintCallable, Category = "DDR|Gait")
	EDDRGait GetCurrentGait() const { return CurrentGait; }

	UFUNCTION(BlueprintCallable, Category = "DDR|Gait")
	void SetWantsSprint(bool bNewWantsSprint);

	// Dash vive no GAS (GA_Dash) — o estado aqui vem da tag State.Movement.Dashing.
	UFUNCTION(BlueprintCallable, Category = "DDR|Movement")
	bool IsDashing() const { return LocomotionState.bIsDashing; }

	UFUNCTION(BlueprintPure, Category = "DDR|Locomotion")
	const FDDRLocomotionState& GetLocomotionState() const { return LocomotionState; }

	/** Trava WASD (launcher/juggle/pin) — zera aceleração para o AnimBP não tocar Run no ar. */
	void SetLocomotionInputBlocked(bool bBlocked);
	bool IsLocomotionInputBlocked() const { return bBlockLocomotionInput; }

	UPROPERTY(EditAnywhere, Category = "DDR|Gait")
	float WalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Gait")
	float RunSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category = "DDR|Gait")
	float SprintSpeed = 750.f;

	// Queda mais pesada que a subida (gravidade assimétrica, doc 13 §3). 1.0 = simétrico.
	UPROPERTY(EditAnywhere, Category = "DDR|Jump", meta = (ClampMin = "1.0"))
	float FallingGravityMultiplier = 1.25f;

protected:
	void UpdateGaitFromInput();
	void UpdateLocomotionState();

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Gait")
	EDDRGait CurrentGait = EDDRGait::Run;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Gait")
	bool bWantsSprint = false;

	UPROPERTY(VisibleInstanceOnly, Category = "DDR|Locomotion")
	FDDRLocomotionState LocomotionState;

	bool bBlockLocomotionInput = false;
};
