// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DDRComboWindow.generated.h"

UCLASS(meta = (DisplayName = "DDR Combo Window"))
class DARKDUNGEONRIFT_API UANS_DDRComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
