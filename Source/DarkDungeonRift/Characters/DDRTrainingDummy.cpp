// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRTrainingDummy.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ADDRTrainingDummy::ADDRTrainingDummy()
{
	AutoPossessAI = EAutoPossessAI::Disabled;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	DummyMesh->SetupAttachment(GetCapsuleComponent());
	DummyMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADDRTrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	if (UDDRAbilitySystemComponent* ASC = Cast<UDDRAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->SetNumericAttributeBase(UDDRAttributeSet::GetMaxHealthAttribute(), 5000.f);
		ASC->SetNumericAttributeBase(UDDRAttributeSet::GetHealthAttribute(), 5000.f);
		ASC->AddLooseGameplayTag(DDRTags::Faction_Enemy);
	}
}
