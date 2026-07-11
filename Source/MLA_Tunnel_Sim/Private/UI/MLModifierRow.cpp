// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLModifierRow.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "UI/MLUiText.h"

void UMLModifierRow::NativeConstruct()
{
	Super::NativeConstruct();

	MLUi::PopulateEnumCombo(ComboBoxString_Type, MLUi::ModifierTypeOptions(), static_cast<int32>(Modifier.Type));

	if (ComboBoxString_Type)
	{
		ComboBoxString_Type->OnSelectionChanged.AddUniqueDynamic(this, &UMLModifierRow::HandleTypeChanged);
	}
	if (SpinBox_Start)
	{
		SpinBox_Start->OnValueCommitted.AddUniqueDynamic(this, &UMLModifierRow::HandleStartCommitted);
	}
	if (SpinBox_Length)
	{
		SpinBox_Length->OnValueCommitted.AddUniqueDynamic(this, &UMLModifierRow::HandleLengthCommitted);
	}
	if (Button_Remove)
	{
		Button_Remove->OnClicked.AddUniqueDynamic(this, &UMLModifierRow::HandleRemoveClicked);
	}
}

void UMLModifierRow::SetModifier(int32 InIndex, const FMLModifierConfig& InModifier)
{
	ModifierIndex = InIndex;
	Modifier = InModifier;

	if (ComboBoxString_Type && ComboBoxString_Type->GetOptionCount() > 0)
	{
		ComboBoxString_Type->SetSelectedIndex(static_cast<int32>(Modifier.Type));
	}
	if (SpinBox_Start)
	{
		SpinBox_Start->SetValue(Modifier.StartFeet);
	}
	if (SpinBox_Length)
	{
		SpinBox_Length->SetValue(Modifier.LengthFeet);
	}
}

void UMLModifierRow::NotifyChanged()
{
	if (ModifierIndex != INDEX_NONE)
	{
		OnModifierChanged.Broadcast(ModifierIndex, Modifier);
	}
}

void UMLModifierRow::HandleTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !ComboBoxString_Type)
	{
		return;
	}

	Modifier.Type = static_cast<EMLModifierType>(MLUi::GetComboIndex(ComboBoxString_Type, MLUi::ModifierTypeOptions().Num()));
	NotifyChanged();
}

void UMLModifierRow::HandleStartCommitted(float InValue, ETextCommit::Type CommitMethod)
{
	Modifier.StartFeet = FMath::Max(0.f, InValue);
	NotifyChanged();
}

void UMLModifierRow::HandleLengthCommitted(float InValue, ETextCommit::Type CommitMethod)
{
	Modifier.LengthFeet = FMath::Max(0.f, InValue);
	NotifyChanged();
}

void UMLModifierRow::HandleRemoveClicked()
{
	if (ModifierIndex != INDEX_NONE)
	{
		OnRemoveRequested.Broadcast(ModifierIndex);
	}
}
