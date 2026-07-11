// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLRelaySwitchRow.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLHardwareBoardsScreen.h"
#include "UI/MLUiText.h"

void UMLRelaySwitchRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Auto)
	{
		Button_Auto->OnClicked.AddUniqueDynamic(this, &UMLRelaySwitchRow::HandleAuto);
	}
	if (Button_On)
	{
		Button_On->OnClicked.AddUniqueDynamic(this, &UMLRelaySwitchRow::HandleOn);
	}
	if (Button_Off)
	{
		Button_Off->OnClicked.AddUniqueDynamic(this, &UMLRelaySwitchRow::HandleOff);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.AddUniqueDynamic(this, &UMLRelaySwitchRow::HandleRelayStateChanged);
	}

	UpdateSwitchUi();
}

void UMLRelaySwitchRow::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.RemoveDynamic(this, &UMLRelaySwitchRow::HandleRelayStateChanged);
	}

	Super::NativeDestruct();
}

UMicrologicControllerSubsystem* UMLRelaySwitchRow::GetController() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}

void UMLRelaySwitchRow::SetRelay(int32 InRelayNumber, const FString& InDescription)
{
	RelayNumber = InRelayNumber;

	if (TextBlock_Relay)
	{
		const FString Label = InDescription.IsEmpty()
			? FString::FromInt(RelayNumber)
			: FString::Printf(TEXT("%d — %s"), RelayNumber, *InDescription);
		TextBlock_Relay->SetText(FText::FromString(Label));
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		UpdateLed(Controller->GetRelayState(RelayNumber));
	}

	UpdateSwitchUi();
}

void UMLRelaySwitchRow::SetPinSource(UMLHardwareBoardsScreen* InPinSource)
{
	PinSource = InPinSource;
}

void UMLRelaySwitchRow::RequestOverride(EMLRelayOverride Override)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		const FString Pin = PinSource.IsValid() ? PinSource->GetPin() : FString();
		// A rejected PIN leaves the switch where it was; re-read either way.
		Controller->SetRelayOverride(RelayNumber, Override, Pin);
	}

	UpdateSwitchUi();
}

void UMLRelaySwitchRow::UpdateSwitchUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	const EMLRelayOverride Current = Controller
		? Controller->GetRelayOverride(RelayNumber)
		: EMLRelayOverride::Auto;

	if (Button_Auto)
	{
		Button_Auto->SetIsEnabled(Current != EMLRelayOverride::Auto);
	}
	if (Button_On)
	{
		Button_On->SetIsEnabled(Current != EMLRelayOverride::ForcedOn);
	}
	if (Button_Off)
	{
		Button_Off->SetIsEnabled(Current != EMLRelayOverride::ForcedOff);
	}
}

void UMLRelaySwitchRow::UpdateLed(bool bOn)
{
	if (Border_Led)
	{
		Border_Led->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
	}
}

void UMLRelaySwitchRow::HandleAuto() { RequestOverride(EMLRelayOverride::Auto); }
void UMLRelaySwitchRow::HandleOn()   { RequestOverride(EMLRelayOverride::ForcedOn); }
void UMLRelaySwitchRow::HandleOff()  { RequestOverride(EMLRelayOverride::ForcedOff); }

void UMLRelaySwitchRow::HandleRelayStateChanged(int32 InRelayNumber, bool bOn)
{
	if (InRelayNumber == RelayNumber)
	{
		UpdateLed(bOn);
	}
}
