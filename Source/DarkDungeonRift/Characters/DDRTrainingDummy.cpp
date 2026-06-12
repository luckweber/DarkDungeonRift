// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRTrainingDummy.h"

#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/SoftObjectPath.h"

ADDRTrainingDummy::ADDRTrainingDummy()
{
	AutoPossessAI = EAutoPossessAI::Disabled;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();

		// SEM Controller o CMC pula PerformMovement (gate interno) — a subida do juggle
		// funciona (SetActorLocation na base), mas a QUEDA (EndAirborne: Falling -3000)
		// e PhysFalling: sem este flag o dummy ficava PENDURADO no ar para sempre.
		// Inimigos do M3 terao AIController e nao precisam disto.
		MoveComp->bRunPhysicsWithNoController = true;
	}

	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	DummyMesh->SetupAttachment(GetCapsuleComponent());
	DummyMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADDRTrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (!SkelMesh->GetPhysicsAsset())
		{
			static const FSoftObjectPath DefaultPAPath(
				TEXT("/Game/Characters/Mannequins/Rigs/PA_Manny.PA_Manny"));
			if (UPhysicsAsset* PA = Cast<UPhysicsAsset>(DefaultPAPath.TryLoad()))
			{
				SkelMesh->SetPhysicsAsset(PA);
				UE_LOG(LogDDR, Log, TEXT("[RAGDOLL] %s Physics Asset atribuido (%s)"),
					*GetName(), *PA->GetName());
			}
			else
			{
				UE_LOG(LogDDR, Warning,
					TEXT("[RAGDOLL] %s sem Physics Asset — assigne PA_Manny no BP (Skeletal Mesh → Physics Asset)"),
					*GetName());
			}
		}
	}

	if (UDDRAbilitySystemComponent* ASC = Cast<UDDRAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->SetNumericAttributeBase(UDDRAttributeSet::GetMaxHealthAttribute(), 5000.f);
		ASC->SetNumericAttributeBase(UDDRAttributeSet::GetHealthAttribute(), 5000.f);
		ASC->AddLooseGameplayTag(DDRTags::Faction_Enemy);
	}
}
