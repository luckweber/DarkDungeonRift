// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_DDRGameplayEvent.generated.h"

// Notify de FRAME DE CONTATO: envia um GameplayEvent pro dono no frame exato da anim.
// As abilities escutam com WaitGameplayEvent — o dano/efeito sincroniza com a ESPADA,
// nao com timers. Uso canonico: tag "Event.Combat.SlamLand" no frame em que a lamina
// toca o chao na secao End da AM_AirSlam (GA_AirSlam auto-detecta e espera por ele).
UCLASS(meta = (DisplayName = "DDR Gameplay Event"))
class DARKDUNGEONRIFT_API UAN_DDRGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "DDR")
	FGameplayTag EventTag;
};
