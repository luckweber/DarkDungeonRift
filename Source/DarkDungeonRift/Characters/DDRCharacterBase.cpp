// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCharacterBase.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRCharacterMovementComponent.h"
#include "DDRCombatComponent.h"

ADDRCharacterBase::ADDRCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDDRCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AbilitySystemComponent = CreateDefaultSubobject<UDDRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UDDRAttributeSet>(TEXT("AttributeSet"));
	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());

	CombatComponent = CreateDefaultSubobject<UDDRCombatComponent>(TEXT("CombatComponent"));
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
