// Copyright Micrologic Associates. All Rights Reserved.

#include "MicrologicDeviceComponent.h"
#include "MicrologicControllerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMicrologicDeviceComponent::UMicrologicDeviceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMicrologicDeviceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.AddDynamic(this, &UMicrologicDeviceComponent::HandleRelayStateChanged);

		const bool bInitial = Controller->GetRelayState(RelayNumber);
		if (bInitial != bEnergized)
		{
			HandleRelayStateChanged(RelayNumber, bInitial);
		}
	}
}

void UMicrologicDeviceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.RemoveDynamic(this, &UMicrologicDeviceComponent::HandleRelayStateChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UMicrologicDeviceComponent::HandleRelayStateChanged(int32 InRelayNumber, bool bOn)
{
	if (InRelayNumber != RelayNumber || bOn == bEnergized)
	{
		return;
	}

	bEnergized = bOn;
	OnSignalChanged.Broadcast(bOn);
	if (bOn)
	{
		OnEnergized.Broadcast();
	}
	else
	{
		OnDeEnergized.Broadcast();
	}
}

UMicrologicControllerSubsystem* UMicrologicDeviceComponent::GetController() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}
