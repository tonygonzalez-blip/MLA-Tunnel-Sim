// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLControllerScreenBase.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"

void UMLControllerScreenBase::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromController();
}

UMicrologicControllerSubsystem* UMLControllerScreenBase::GetController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UMicrologicControllerSubsystem>();
}
