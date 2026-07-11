// Copyright Micrologic Associates. All Rights Reserved.

#include "MicrologicSensorComponent.h"
#include "MicrologicControllerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMicrologicSensorComponent::UMicrologicSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMicrologicSensorComponent::SetTriggered(bool bTriggered)
{
	bLastState = bTriggered;

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	if (Channel > 0)
	{
		Controller->SetInputRaw(Channel, bTriggered);
	}
	else if (InputType != EMLInputType::None)
	{
		Controller->SetInputRawByType(InputType, bTriggered);
	}
}

UMicrologicControllerSubsystem* UMicrologicSensorComponent::GetController() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}
