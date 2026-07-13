// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLRelayEditPanel.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLModifierRow.h"
#include "UI/MLUiText.h"

void UMLRelayEditPanel::NativeConstruct()
{
	Super::NativeConstruct();

	MLUi::PopulateEnumCombo(ComboBoxString_Type, MLUi::RelayTypeOptions(), static_cast<int32>(Working.Type));
	MLUi::PopulateEnumCombo(ComboBoxString_FunctionType, MLUi::FunctionTypeOptions(), static_cast<int32>(Working.Function.Type));
	MLUi::PopulateEnumCombo(ComboBoxString_TurnOnRef, MLUi::TurnReferenceOptions(), static_cast<int32>(Working.Function.TurnOnReference));
	MLUi::PopulateEnumCombo(ComboBoxString_TurnOffRef, MLUi::TurnReferenceOptions(), static_cast<int32>(Working.Function.TurnOffReference));

	if (Button_AddModifier)
	{
		Button_AddModifier->OnClicked.AddUniqueDynamic(this, &UMLRelayEditPanel::HandleAddModifierClicked);
	}
	if (Button_Save)
	{
		Button_Save->OnClicked.AddUniqueDynamic(this, &UMLRelayEditPanel::HandleSaveClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.AddUniqueDynamic(this, &UMLRelayEditPanel::HandleCancelClicked);
	}
}

UMicrologicControllerSubsystem* UMLRelayEditPanel::GetController() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}

void UMLRelayEditPanel::OpenForRelay(const FMLRelayConfig& InRelay)
{
	Working = InRelay;
	Open();
}

void UMLRelayEditPanel::OpenForNew(int32 SuggestedNumber)
{
	Working = FMLRelayConfig();
	Working.RelayNumber = FMath::Max(1, SuggestedNumber);
	Working.bActive = true;
	Open();
}

void UMLRelayEditPanel::Open()
{
	LoadWorkingToUi();
	SetVisibility(ESlateVisibility::Visible);
}

void UMLRelayEditPanel::LoadWorkingToUi()
{
	if (TextBlock_RelayNumber)
	{
		TextBlock_RelayNumber->SetText(FText::AsNumber(Working.RelayNumber));
	}
	if (CheckBox_Active)
	{
		CheckBox_Active->SetIsChecked(Working.bActive);
	}
	if (EditableTextBox_Description)
	{
		EditableTextBox_Description->SetText(FText::FromString(Working.Description));
	}
	if (CheckBox_Default)
	{
		CheckBox_Default->SetIsChecked(Working.bDefault);
	}
	if (ComboBoxString_Type && ComboBoxString_Type->GetOptionCount() > 0)
	{
		ComboBoxString_Type->SetSelectedIndex(static_cast<int32>(Working.Type));
	}
	if (CheckBox_InactivityCheck)
	{
		CheckBox_InactivityCheck->SetIsChecked(Working.bInactivityCheck);
	}
	if (SpinBox_InterlockStart)
	{
		SpinBox_InterlockStart->SetValue(Working.InterlockStartSeconds);
	}
	if (SpinBox_InterlockStop)
	{
		SpinBox_InterlockStop->SetValue(Working.InterlockStopSeconds);
	}
	if (SpinBox_LookBack)
	{
		SpinBox_LookBack->SetValue(Working.LookBackFeet);
	}
	if (ComboBoxString_FunctionType && ComboBoxString_FunctionType->GetOptionCount() > 0)
	{
		ComboBoxString_FunctionType->SetSelectedIndex(static_cast<int32>(Working.Function.Type));
	}
	if (SpinBox_DevicePosition)
	{
		SpinBox_DevicePosition->SetValue(Working.Function.DevicePositionFeet);
	}
	if (SpinBox_TurnOnFeet)
	{
		SpinBox_TurnOnFeet->SetValue(Working.Function.TurnOnFeet);
	}
	if (ComboBoxString_TurnOnRef && ComboBoxString_TurnOnRef->GetOptionCount() > 0)
	{
		ComboBoxString_TurnOnRef->SetSelectedIndex(static_cast<int32>(Working.Function.TurnOnReference));
	}
	if (SpinBox_TurnOffFeet)
	{
		SpinBox_TurnOffFeet->SetValue(Working.Function.TurnOffFeet);
	}
	if (ComboBoxString_TurnOffRef && ComboBoxString_TurnOffRef->GetOptionCount() > 0)
	{
		ComboBoxString_TurnOffRef->SetSelectedIndex(static_cast<int32>(Working.Function.TurnOffReference));
	}

	RebuildModifierRows();
}

void UMLRelayEditPanel::PullUiToWorking()
{
	if (CheckBox_Active)
	{
		Working.bActive = CheckBox_Active->IsChecked();
	}
	if (EditableTextBox_Description)
	{
		Working.Description = EditableTextBox_Description->GetText().ToString();
	}
	if (CheckBox_Default)
	{
		Working.bDefault = CheckBox_Default->IsChecked();
	}
	if (ComboBoxString_Type)
	{
		Working.Type = static_cast<EMLRelayType>(MLUi::GetComboIndex(ComboBoxString_Type, MLUi::RelayTypeOptions().Num()));
	}
	if (CheckBox_InactivityCheck)
	{
		Working.bInactivityCheck = CheckBox_InactivityCheck->IsChecked();
	}
	if (SpinBox_InterlockStart)
	{
		Working.InterlockStartSeconds = FMath::Max(0.f, SpinBox_InterlockStart->GetValue());
	}
	if (SpinBox_InterlockStop)
	{
		Working.InterlockStopSeconds = FMath::Max(0.f, SpinBox_InterlockStop->GetValue());
	}
	if (SpinBox_LookBack)
	{
		Working.LookBackFeet = FMath::Max(0.f, SpinBox_LookBack->GetValue());
	}
	if (ComboBoxString_FunctionType)
	{
		Working.Function.Type = static_cast<EMLFunctionType>(MLUi::GetComboIndex(ComboBoxString_FunctionType, MLUi::FunctionTypeOptions().Num()));
	}
	if (SpinBox_DevicePosition)
	{
		Working.Function.DevicePositionFeet = FMath::Max(0.f, SpinBox_DevicePosition->GetValue());
	}
	if (SpinBox_TurnOnFeet)
	{
		Working.Function.TurnOnFeet = FMath::Max(0.f, SpinBox_TurnOnFeet->GetValue());
	}
	if (ComboBoxString_TurnOnRef)
	{
		Working.Function.TurnOnReference = static_cast<EMLTurnReference>(MLUi::GetComboIndex(ComboBoxString_TurnOnRef, MLUi::TurnReferenceOptions().Num()));
	}
	if (SpinBox_TurnOffFeet)
	{
		Working.Function.TurnOffFeet = FMath::Max(0.f, SpinBox_TurnOffFeet->GetValue());
	}
	if (ComboBoxString_TurnOffRef)
	{
		Working.Function.TurnOffReference = static_cast<EMLTurnReference>(MLUi::GetComboIndex(ComboBoxString_TurnOffRef, MLUi::TurnReferenceOptions().Num()));
	}
	// Working.Function.Modifiers is kept current by the modifier-row events.
}

void UMLRelayEditPanel::RebuildModifierRows()
{
	if (!ScrollBox_Modifiers)
	{
		return;
	}

	ScrollBox_Modifiers->ClearChildren();

	if (!ModifierRowClass)
	{
		return;
	}

	for (int32 Index = 0; Index < Working.Function.Modifiers.Num(); ++Index)
	{
		if (UMLModifierRow* Row = CreateWidget<UMLModifierRow>(this, ModifierRowClass))
		{
			Row->OnModifierChanged.AddUniqueDynamic(this, &UMLRelayEditPanel::HandleModifierChanged);
			Row->OnRemoveRequested.AddUniqueDynamic(this, &UMLRelayEditPanel::HandleModifierRemove);
			ScrollBox_Modifiers->AddChild(Row);
			Row->SetModifier(Index, Working.Function.Modifiers[Index]);
		}
	}
}

void UMLRelayEditPanel::HandleModifierChanged(int32 ModifierIndex, FMLModifierConfig Modifier)
{
	if (Working.Function.Modifiers.IsValidIndex(ModifierIndex))
	{
		Working.Function.Modifiers[ModifierIndex] = Modifier;
	}
}

void UMLRelayEditPanel::HandleModifierRemove(int32 ModifierIndex)
{
	if (Working.Function.Modifiers.IsValidIndex(ModifierIndex))
	{
		Working.Function.Modifiers.RemoveAt(ModifierIndex);
		RebuildModifierRows();
	}
}

void UMLRelayEditPanel::HandleAddModifierClicked()
{
	Working.Function.Modifiers.AddDefaulted();
	RebuildModifierRows();
}

void UMLRelayEditPanel::HandleSaveClicked()
{
	PullUiToWorking();

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->UpsertStagedRelay(Working);
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnSaved.Broadcast();
}

void UMLRelayEditPanel::HandleCancelClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
