// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DDRAbilityInput.h"
#include "DDRAbilitySystemComponent.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRCombatComponent.h"
#include "DDRLog.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GA_AirAttack.h"
#include "GA_AirSlam.h"
#include "GA_AttackLight.h"
#include "GA_Dash.h"
#include "GA_Launcher.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

ADDRPlayerCharacter::ADDRPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UDDRCharacterMovementComponent* DDRMove = GetDDRMovement())
	{
		DDRMove->bOrientRotationToMovement = true;
		DDRMove->RotationRate = FRotator(0.f, 720.f, 0.f);
		DDRMove->JumpZVelocity = 700.f;
		DDRMove->AirControl = 0.35f;
		DDRMove->SetGait(EDDRGait::Run);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 950.f;
	CameraBoom->SetRelativeRotation(FRotator(-55.f, -45.f, 0.f));
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	StartupAbilities.Add({ UGA_AttackLight::StaticClass(), EDDRAbilityInputID::Attack });
	StartupAbilities.Add({ UGA_AirAttack::StaticClass(), EDDRAbilityInputID::Attack }); // mesmo botao; gate por InAir
	StartupAbilities.Add({ UGA_Dash::StaticClass(), EDDRAbilityInputID::Dash });
	StartupAbilities.Add({ UGA_Launcher::StaticClass(), EDDRAbilityInputID::Launcher });
	StartupAbilities.Add({ UGA_AirSlam::StaticClass(), EDDRAbilityInputID::AirSlam });
}

void ADDRPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogDDR, Warning, TEXT("DefaultMappingContext not assigned on %s — assign IMC_Default in BP_DDRPlayer."), *GetName());
			}
		}
	}
}

void ADDRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADDRPlayerCharacter::Move);
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ADDRPlayerCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ADDRPlayerCharacter::StopSprint);
		}

		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ADDRPlayerCharacter::OnAttackPressed);
		}

		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ADDRPlayerCharacter::OnDashPressed);
		}

		if (LauncherAction)
		{
			EnhancedInputComponent->BindAction(LauncherAction, ETriggerEvent::Started, this, &ADDRPlayerCharacter::OnLauncherPressed);
		}

		if (AirSlamAction)
		{
			EnhancedInputComponent->BindAction(AirSlamAction, ETriggerEvent::Started, this, &ADDRPlayerCharacter::OnAirSlamPressed);
		}
	}
	else
	{
		UE_LOG(LogDDR, Error, TEXT("Enhanced Input component missing on %s"), *GetNameSafe(this));
	}
}

void ADDRPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.IsNearlyZero() || !Controller || !CameraBoom)
	{
		return;
	}

	if (const UDDRCombatComponent* Combat = FindComponentByClass<UDDRCombatComponent>())
	{
		if (Combat->IsAirHorizontalInputLocked())
		{
			return;
		}
	}

	const float CamYaw = CameraBoom->GetComponentRotation().Yaw;
	const FRotator YawRot(0.f, CamYaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Input.Y);
	AddMovementInput(Right, Input.X);
}

void ADDRPlayerCharacter::StartSprint()
{
	if (UDDRCharacterMovementComponent* DDRMove = GetDDRMovement())
	{
		DDRMove->SetWantsSprint(true);
	}
}

void ADDRPlayerCharacter::StopSprint()
{
	if (UDDRCharacterMovementComponent* DDRMove = GetDDRMovement())
	{
		DDRMove->SetWantsSprint(false);
	}
}

void ADDRPlayerCharacter::OnAttackPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EDDRAbilityInputID::Attack));
	}
}

void ADDRPlayerCharacter::OnDashPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EDDRAbilityInputID::Dash));
	}
}

void ADDRPlayerCharacter::OnLauncherPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EDDRAbilityInputID::Launcher));
	}
}

void ADDRPlayerCharacter::OnAirSlamPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EDDRAbilityInputID::AirSlam));
	}
}
