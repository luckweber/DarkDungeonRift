// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRCharacterBase.h"
#include "DDRTrainingDummy.generated.h"

class UStaticMeshComponent;

UCLASS()
class DARKDUNGEONRIFT_API ADDRTrainingDummy : public ADDRCharacterBase
{
	GENERATED_BODY()

public:
	ADDRTrainingDummy();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> DummyMesh;
};
