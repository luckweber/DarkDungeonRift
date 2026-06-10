// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterBase.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRCharacterMovementComponent.h"

ADDRCharacterBase::ADDRCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDDRCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AbilitySystemComponent = CreateDefaultSubobject<UDDRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UDDRAttributeSet>(TEXT("AttributeSet"));
	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());
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
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}
