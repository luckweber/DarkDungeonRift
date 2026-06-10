// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRGameMode.h"

#include "DDRPlayerCharacter.h"
#include "DDRPlayerController.h"

ADDRGameMode::ADDRGameMode()
{
	DefaultPawnClass = ADDRPlayerCharacter::StaticClass();
	PlayerControllerClass = ADDRPlayerController::StaticClass();
}
