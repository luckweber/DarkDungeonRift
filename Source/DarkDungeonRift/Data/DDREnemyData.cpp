// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDREnemyData.h"

FPrimaryAssetId UDDREnemyData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("DDREnemyData"), GetFName());
}
