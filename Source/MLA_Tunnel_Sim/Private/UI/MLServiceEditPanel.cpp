// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLServiceEditPanel.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLUiText.h"

void UMLServiceEditPanel::NativeConstruct()
{
	Super::NativeConstruct();

	MLUi::PopulateEnumCombo(ComboBoxString_Type, MLUi::ServiceTypeOptions(), static_cast<int32>(Working.Type));
	MLUi::PopulateEnumCombo(ComboBoxString_Command, MLUi::CommandOptions(), static_cast<int32>(Working.Command));

	if (Button_Save)
	{
		Button_Save->OnClicked.AddUniqueDynamic(this, &UMLServiceEditPanel::HandleSaveClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.AddUniqueDynamic(this, &UMLServiceEditPanel::HandleCancelClicked);
	}
	if (Button_Delete)
	{
		Button_Delete->OnClicked.AddUniqueDynamic(this, &UMLServiceEditPanel::HandleDeleteClicked);
	}
}

UMicrologicControllerSubsystem* UMLServiceEditPanel::GetController() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}

void UMLServiceEditPanel::OpenForService(const FMLServiceConfig& InService)
{
	Working = InService;
	Open();
}

void UMLServiceEditPanel::OpenForNew(int32 SuggestedNumber)
{
	Working = FMLServiceConfig();
	Working.ServiceNumber = FMath::Max(1, SuggestedNumber);
	Open();
}

void UMLServiceEditPanel::Open()
{
	LoadWorkingToUi();
	SetVisibility(ESlateVisibility::Visible);
}

void UMLServiceEditPanel::LoadWorkingToUi()
{
	if (SpinBox_ServiceNumber)
	{
		SpinBox_ServiceNumber->SetValue(static_cast<float>(Working.ServiceNumber));
	}
	if (EditableTextBox_Description)
	{
		EditableTextBox_Description->SetText(FText::FromString(Working.Description));
	}
	if (ComboBoxString_Type && ComboBoxString_Type->GetOptionCount() > 0)
	{
		ComboBoxString_Type->SetSelectedIndex(static_cast<int32>(Working.Type));
	}
	if (EditableTextBox_Relays)
	{
		EditableTextBox_Relays->SetText(FText::FromString(MLUi::IntArrayToCsv(Working.RelayNumbers)));
	}
	if (SpinBox_Time)
	{
		SpinBox_Time->SetValue(Working.TimeSeconds);
	}
	if (SpinBox_Delay)
	{
		SpinBox_Delay->SetValue(Working.DelaySeconds);
	}
	if (EditableTextBox_MacroServices)
	{
		EditableTextBox_MacroServices->SetText(FText::FromString(MLUi::IntArrayToCsv(Working.MacroServiceNumbers)));
	}
	if (ComboBoxString_Command && ComboBoxString_Command->GetOptionCount() > 0)
	{
		ComboBoxString_Command->SetSelectedIndex(static_cast<int32>(Working.Command));
	}
}

void UMLServiceEditPanel::PullUiToWorking()
{
	if (SpinBox_ServiceNumber)
	{
		Working.ServiceNumber = FMath::Max(1, FMath::RoundToInt32(SpinBox_ServiceNumber->GetValue()));
	}
	if (EditableTextBox_Description)
	{
		Working.Description = EditableTextBox_Description->GetText().ToString();
	}
	if (ComboBoxString_Type)
	{
		Working.Type = static_cast<EMLServiceType>(MLUi::GetComboIndex(ComboBoxString_Type, MLUi::ServiceTypeOptions().Num()));
	}
	if (EditableTextBox_Relays)
	{
		Working.RelayNumbers = MLUi::CsvToIntArray(EditableTextBox_Relays->GetText().ToString());
	}
	if (SpinBox_Time)
	{
		Working.TimeSeconds = FMath::Max(0.f, SpinBox_Time->GetValue());
	}
	if (SpinBox_Delay)
	{
		Working.DelaySeconds = FMath::Max(0.f, SpinBox_Delay->GetValue());
	}
	if (EditableTextBox_MacroServices)
	{
		Working.MacroServiceNumbers = MLUi::CsvToIntArray(EditableTextBox_MacroServices->GetText().ToString());
	}
	if (ComboBoxString_Command)
	{
		Working.Command = static_cast<EMLCommand>(MLUi::GetComboIndex(ComboBoxString_Command, MLUi::CommandOptions().Num()));
	}
}

void UMLServiceEditPanel::HandleSaveClicked()
{
	PullUiToWorking();

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->UpsertStagedService(Working);
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnSaved.Broadcast();
}

void UMLServiceEditPanel::HandleCancelClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMLServiceEditPanel::HandleDeleteClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->RemoveStagedService(Working.ServiceNumber);
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnSaved.Broadcast();
}
