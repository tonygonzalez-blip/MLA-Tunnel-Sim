// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLInputRow.h"

#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLUiText.h"

void UMLInputRow::NativeConstruct()
{
	Super::NativeConstruct();

	MLUi::PopulateEnumCombo(ComboBoxString_Type, MLUi::InputTypeOptions(), static_cast<int32>(Config.Type));

	if (EditableTextBox_Description)
	{
		EditableTextBox_Description->OnTextCommitted.AddUniqueDynamic(this, &UMLInputRow::HandleDescriptionCommitted);
	}
	if (ComboBoxString_Type)
	{
		ComboBoxString_Type->OnSelectionChanged.AddUniqueDynamic(this, &UMLInputRow::HandleTypeChanged);
	}
	if (CheckBox_Inverted)
	{
		CheckBox_Inverted->OnCheckStateChanged.AddUniqueDynamic(this, &UMLInputRow::HandleInvertedChanged);
	}
	if (SpinBox_Debounce)
	{
		SpinBox_Debounce->OnValueCommitted.AddUniqueDynamic(this, &UMLInputRow::HandleDebounceCommitted);
	}
	if (SpinBox_TriggerService)
	{
		SpinBox_TriggerService->OnValueCommitted.AddUniqueDynamic(this, &UMLInputRow::HandleTriggerServiceCommitted);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnInputStateChanged.AddUniqueDynamic(this, &UMLInputRow::HandleInputStateChanged);
	}
}

void UMLInputRow::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnInputStateChanged.RemoveDynamic(this, &UMLInputRow::HandleInputStateChanged);
	}

	Super::NativeDestruct();
}

UMicrologicControllerSubsystem* UMLInputRow::GetController() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}

void UMLInputRow::SetInput(const FMLInputConfig& InConfig)
{
	Config = InConfig;

	if (TextBlock_Channel)
	{
		TextBlock_Channel->SetText(FText::AsNumber(Config.Channel));
	}
	if (EditableTextBox_Description)
	{
		EditableTextBox_Description->SetText(FText::FromString(Config.Description));
	}
	if (ComboBoxString_Type && ComboBoxString_Type->GetOptionCount() > 0)
	{
		// Fires OnSelectionChanged with ESelectInfo::Direct; the handler ignores that.
		ComboBoxString_Type->SetSelectedIndex(static_cast<int32>(Config.Type));
	}
	if (CheckBox_Inverted)
	{
		CheckBox_Inverted->SetIsChecked(Config.bInverted);
	}
	if (SpinBox_Debounce)
	{
		SpinBox_Debounce->SetValue(Config.DebounceSeconds);
	}
	if (SpinBox_TriggerService)
	{
		SpinBox_TriggerService->SetValue(static_cast<float>(Config.TriggerServiceNumber));
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		UpdateLed(Controller->GetInputState(Config.Channel));
	}
}

void UMLInputRow::PushToStaged()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->UpsertStagedInput(Config);
	}
}

void UMLInputRow::UpdateLed(bool bOn)
{
	if (Border_Led)
	{
		Border_Led->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
	}
}

void UMLInputRow::HandleDescriptionCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	Config.Description = Text.ToString();
	PushToStaged();
}

void UMLInputRow::HandleTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// Ignore programmatic SetSelectedIndex calls from SetInput.
	if (SelectionType == ESelectInfo::Direct || !ComboBoxString_Type)
	{
		return;
	}

	Config.Type = static_cast<EMLInputType>(MLUi::GetComboIndex(ComboBoxString_Type, MLUi::InputTypeOptions().Num()));
	PushToStaged();
}

void UMLInputRow::HandleInvertedChanged(bool bIsChecked)
{
	Config.bInverted = bIsChecked;
	PushToStaged();
}

void UMLInputRow::HandleDebounceCommitted(float InValue, ETextCommit::Type CommitMethod)
{
	Config.DebounceSeconds = FMath::Max(0.f, InValue);
	PushToStaged();
}

void UMLInputRow::HandleTriggerServiceCommitted(float InValue, ETextCommit::Type CommitMethod)
{
	Config.TriggerServiceNumber = FMath::Max(0, FMath::RoundToInt32(InValue));
	PushToStaged();
}

void UMLInputRow::HandleInputStateChanged(int32 Channel, bool bOn)
{
	if (Channel == Config.Channel)
	{
		UpdateLed(bOn);
	}
}
