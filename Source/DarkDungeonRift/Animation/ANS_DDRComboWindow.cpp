// Copyright Epic Games, Inc. All Rights Reserved.

#include "ANS_DDRComboWindow.h"

#include "AbilitySystemComponent.h"
#include "DDRAbilityInput.h"
#include "DDRCombatComponent.h"
#include "DDRLog.h"

void UANS_DDRComboWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	if (UDDRCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UDDRCombatComponent>())
	{
		Combat->OpenComboWindow();

		if (Combat->HasBufferedAttack())
		{
			UE_LOG(LogDDR, Log, TEXT("[ATK] buffer CONSUMIDO na abertura da janela"));
			if (UAbilitySystemComponent* ASC = MeshComp->GetOwner()->FindComponentByClass<UAbilitySystemComponent>())
			{
				ASC->AbilityLocalInputPressed(static_cast<int32>(EDDRAbilityInputID::Attack));
			}
			Combat->ClearBufferedAttack();
		}
	}
}

void UANS_DDRComboWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	if (UDDRCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UDDRCombatComponent>())
	{
		Combat->CloseComboWindow();
	}
}
