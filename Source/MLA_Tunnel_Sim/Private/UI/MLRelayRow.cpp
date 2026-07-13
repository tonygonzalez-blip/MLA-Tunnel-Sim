// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLRelayRow.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLUiText.h"

void UMLRelayRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Edit)
	{
		Button_Edit->OnClicked.AddUniqueDynamic(this, &UMLRelayRow::HandleEditClicked);
	}
	if (Button_SwitchAuto)
	{
		Button_SwitchAuto->OnClicked.AddUniqueDynamic(this, &UMLRelayRow::HandleSwitchAuto);
	}
	if (Button_SwitchOn)
	{
		Button_SwitchOn->OnClicked.AddUniqueDynamic(this, &UMLRelayRow::HandleSwitchOn);
	}
	if (Button_SwitchOff)
	{
		Button_SwitchOff->OnClicked.AddUniqueDynamic(this, &UMLRelayRow::HandleSwitchOff);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.AddUniqueDynamic(this, &UMLRelayRow::HandleRelayStateChanged);
	}

	UpdateSwitchUi();
}

void UMLRelayRow::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.RemoveDynamic(this, &UMLRelayRow::HandleRelayStateChanged);
	}

	Super::NativeDestruct();
}

UMicrologicControllerSubsystem* UMLRelayRow::GetController() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}

void UMLRelayRow::SetRelay(const FMLRelayConfig& InConfig)
{
	Config = InConfig;

	if (TextBlock_Number)
	{
		TextBlock_Number->SetText(FText::AsNumber(Config.RelayNumber));
	}
	if (TextBlock_Description)
	{
		TextBlock_Description->SetText(FText::FromString(Config.Description));
	}
	if (TextBlock_Type)
	{
		const TArray<FString>& Options = MLUi::RelayTypeOptions();
		const int32 TypeIndex = FMath::Clamp(static_cast<int32>(Config.Type), 0, Options.Num() - 1);
		TextBlock_Type->SetText(FText::FromString(Options[TypeIndex]));
	}
	if (TextBlock_Default)
	{
		TextBlock_Default->SetText(FText::FromString(Config.bDefault ? TEXT("Yes") : TEXT("No")));
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		UpdateLed(Controller->GetRelayState(Config.RelayNumber));
	}

	UpdateSwitchUi();
}

void UMLRelayRow::RequestOverride(EMLRelayOverride Override)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		const FString Pin = EditableTextBox_Pin ? EditableTextBox_Pin->GetText().ToString() : FString();
		// If the PIN is rejected the controller keeps the old position; the
		// switch UI is re-read from the actual state either way.
		Controller->SetRelayOverride(Config.RelayNumber, Override, Pin);
	}

	UpdateSwitchUi();
}

void UMLRelayRow::UpdateSwitchUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	const EMLRelayOverride Current = Controller
		? Controller->GetRelayOverride(Config.RelayNumber)
		: EMLRelayOverride::Auto;

	if (Button_SwitchAuto)
	{
		Button_SwitchAuto->SetIsEnabled(Current != EMLRelayOverride::Auto);
	}
	if (Button_SwitchOn)
	{
		Button_SwitchOn->SetIsEnabled(Current != EMLRelayOverride::ForcedOn);
	}
	if (Button_SwitchOff)
	{
		Button_SwitchOff->SetIsEnabled(Current != EMLRelayOverride::ForcedOff);
	}
}

void UMLRelayRow::UpdateLed(bool bOn)
{
	if (Border_Led)
	{
		Border_Led->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
	}
}

void UMLRelayRow::HandleEditClicked()
{
	OnEditRequested.Broadcast(Config.RelayNumber);
}

void UMLRelayRow::HandleSwitchAuto() { RequestOverride(EMLRelayOverride::Auto); }
void UMLRelayRow::HandleSwitchOn()   { RequestOverride(EMLRelayOverride::ForcedOn); }
void UMLRelayRow::HandleSwitchOff()  { RequestOverride(EMLRelayOverride::ForcedOff); }

void UMLRelayRow::HandleRelayStateChanged(int32 RelayNumber, bool bOn)
{
	if (RelayNumber == Config.RelayNumber)
	{
		UpdateLed(bOn);
	}
}
