// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRProjectileData.h"

FPrimaryAssetId UDDRProjectileData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("DDRProjectileData"), GetFName());
}
