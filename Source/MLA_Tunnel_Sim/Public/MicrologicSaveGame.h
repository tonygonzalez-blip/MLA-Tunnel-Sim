// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MicrologicTypes.h"
#include "MicrologicSaveGame.generated.h"

/** Persists the controller configuration between sessions. */
UCLASS()
class MLA_TUNNEL_SIM_API UMicrologicSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FMLControllerConfig Config;

	/** Bumped when the config layout changes incompatibly. */
	UPROPERTY()
	int32 ConfigVersion = 1;
};
