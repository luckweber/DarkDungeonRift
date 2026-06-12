// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDREnemyCharacter.h"

#include "DDREnemyAIController.h"
#include "DDREnemyData.h"
#include "DDRAbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ADDREnemyCharacter::ADDREnemyCharacter()
{
	AIControllerClass = ADDREnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	}
}

void ADDREnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyData)
	{
		ApplyEnemyData(EnemyData);
	}
}

void ADDREnemyCharacter::ApplyEnemyData(UDDREnemyData* Data)
{
	if (!Data)
	{
		return;
	}

	EnemyData = Data;
	MeleeAttackRange = Data->MeleeAttackRange;
	DesiredCombatRange = Data->DesiredCombatRange;
	MinRangedRange = Data->MinRangedRange;

	if (USkeletalMesh* MeshAsset = Data->Mesh.LoadSynchronous())
	{
		GetMesh()->SetSkeletalMesh(MeshAsset);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = Data->MoveSpeed;
	}

	if (AttributeSet)
	{
		AttributeSet->InitMaxHealth(Data->Health);
		AttributeSet->InitHealth(Data->Health);
		AttributeSet->InitPoiseMax(Data->PoiseMax);
		AttributeSet->InitPoise(Data->PoiseMax);
		AttributeSet->InitAttackPower(Data->AttackPower);
		AttributeSet->PoiseRegenPerSecond = Data->PoiseRegenPerSecond;
		AttributeSet->PoiseRegenDelaySeconds = Data->PoiseRegenDelaySeconds;
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(DDRTags::Faction_Enemy);

		for (const TSubclassOf<UGameplayAbility>& AbilityClass : Data->Abilities)
		{
			if (AbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
			}
		}
	}

	UE_LOG(LogDDR, Log, TEXT("[ENEMY] %s aplicou data %s HP=%.0f Poise=%.0f"),
		*GetName(), *Data->GetName(), Data->Health, Data->PoiseMax);
}
