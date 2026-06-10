// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "DDRCombatTypes.h"
#include "ANS_DDRHitbox.generated.h"

UCLASS(meta = (DisplayName = "DDR Hitbox"))
class DARKDUNGEONRIFT_API UANS_DDRHitbox : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "DDR|Combat")
	FDDRMeleeSweepParams SweepParams;
};
