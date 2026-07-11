// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLHardwareBoardsScreen.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLRelaySwitchRow.h"
#include "UI/MLUiText.h"

namespace
{
	constexpr int32 LedGridColumns = 8;
	constexpr float PulseLitSeconds = 0.15f;
}

FString UMLHardwareBoardsScreen::GetPin() const
{
	return EditableTextBox_Pin ? EditableTextBox_Pin->GetText().ToString() : FString();
}

void UMLHardwareBoardsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Start)
	{
		Button_Start->OnClicked.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleStartClicked);
	}

	if (CheckBox_Stop1)
	{
		CheckBox_Stop1->OnCheckStateChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleStop1Changed);
	}
	if (CheckBox_Stop2)
	{
		CheckBox_Stop2->OnCheckStateChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleStop2Changed);
	}
	if (CheckBox_Stop3)
	{
		CheckBox_Stop3->OnCheckStateChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleStop3Changed);
	}
	if (CheckBox_Stop4)
	{
		CheckBox_Stop4->OnCheckStateChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleStop4Changed);
	}
	if (CheckBox_Stop5)
	{
		CheckBox_Stop5->OnCheckStateChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleStop5Changed);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnInputStateChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleInputStateChanged);
		Controller->OnConfigChanged.AddUniqueDynamic(this, &UMLHardwareBoardsScreen::HandleConfigChanged);
	}
}

void UMLHardwareBoardsScreen::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnInputStateChanged.RemoveDynamic(this, &UMLHardwareBoardsScreen::HandleInputStateChanged);
		Controller->OnConfigChanged.RemoveDynamic(this, &UMLHardwareBoardsScreen::HandleConfigChanged);
	}

	Super::NativeDestruct();
}

void UMLHardwareBoardsScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Pulse LED blinks: lit only within a short window after each pulse.
	if (Border_PulseLed)
	{
		const bool bLit = Controller->GetTimeSinceLastPulse() < PulseLitSeconds;
		Border_PulseLed->SetBrushColor(bLit ? MLUi::OnColor : MLUi::OffColor);
	}

	// Motor starter LED mirrors the conveyor running state.
	if (Border_StarterLed)
	{
		Border_StarterLed->SetBrushColor(Controller->IsConveyorRunning() ? MLUi::OnColor : MLUi::OffColor);
	}

	// Horn LED lit while the conveyor state machine is in Horn.
	if (Border_HornLed)
	{
		const bool bHorn = Controller->GetConveyorState() == EMLConveyorState::Horn;
		Border_HornLed->SetBrushColor(bHorn ? MLUi::OnColor : MLUi::OffColor);
	}
}

void UMLHardwareBoardsScreen::RefreshFromController()
{
	RebuildInputLeds();
	RebuildRelaySwitches();
	SyncStopCheckBoxes();
}

void UMLHardwareBoardsScreen::RebuildInputLeds()
{
	if (!UniformGridPanel_InputLeds || !WidgetTree)
	{
		return;
	}

	UniformGridPanel_InputLeds->ClearChildren();
	InputLedsByChannel.Empty();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Descriptions for tooltips, from the live config.
	TMap<int32, FString> Descriptions;
	for (const FMLInputConfig& Input : Controller->GetLiveConfig().Inputs)
	{
		Descriptions.Add(Input.Channel, Input.Description);
	}

	for (int32 Channel = 1; Channel <= NumInputLeds; ++Channel)
	{
		UBorder* Led = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		if (!Led)
		{
			continue;
		}

		Led->SetBrushColor(Controller->GetInputState(Channel) ? MLUi::OnColor : MLUi::OffColor);
		Led->SetPadding(FMargin(6.f, 3.f));
		if (const FString* Description = Descriptions.Find(Channel))
		{
			if (!Description->IsEmpty())
			{
				Led->SetToolTipText(FText::FromString(*Description));
			}
		}

		if (UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Label->SetText(FText::AsNumber(Channel));
			Label->SetJustification(ETextJustify::Center);
			Led->SetContent(Label);
		}

		const int32 CellIndex = Channel - 1;
		if (UUniformGridSlot* GridSlot = UniformGridPanel_InputLeds->AddChildToUniformGrid(Led, CellIndex / LedGridColumns, CellIndex % LedGridColumns))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}

		InputLedsByChannel.Add(Channel, Led);
	}
}

void UMLHardwareBoardsScreen::RebuildRelaySwitches()
{
	if (!ScrollBox_RelaySwitches)
	{
		return;
	}

	ScrollBox_RelaySwitches->ClearChildren();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller || !RelaySwitchRowClass)
	{
		return;
	}

	// One row per relay in the LIVE config — this view is the physical board.
	for (const FMLRelayConfig& Relay : Controller->GetLiveConfig().Relays)
	{
		if (UMLRelaySwitchRow* Row = CreateWidget<UMLRelaySwitchRow>(this, RelaySwitchRowClass))
		{
			Row->SetPinSource(this);
			ScrollBox_RelaySwitches->AddChild(Row);
			Row->SetRelay(Relay.RelayNumber, Relay.Description);
		}
	}
}

void UMLHardwareBoardsScreen::SyncStopCheckBoxes()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Checked = circuit closed (stop circuits are Normally Closed).
	if (CheckBox_Stop1)
	{
		CheckBox_Stop1->SetIsChecked(Controller->IsStopCircuitClosed(1));
	}
	if (CheckBox_Stop2)
	{
		CheckBox_Stop2->SetIsChecked(Controller->IsStopCircuitClosed(2));
	}
	if (CheckBox_Stop3)
	{
		CheckBox_Stop3->SetIsChecked(Controller->IsStopCircuitClosed(3));
	}
	if (CheckBox_Stop4)
	{
		CheckBox_Stop4->SetIsChecked(Controller->IsStopCircuitClosed(4));
	}
	if (CheckBox_Stop5)
	{
		CheckBox_Stop5->SetIsChecked(Controller->IsStopCircuitClosed(5));
	}
}

void UMLHardwareBoardsScreen::HandleInputStateChanged(int32 Channel, bool bOn)
{
	if (const TObjectPtr<UBorder>* Led = InputLedsByChannel.Find(Channel))
	{
		if (*Led)
		{
			(*Led)->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
		}
	}
}

void UMLHardwareBoardsScreen::HandleConfigChanged()
{
	RefreshFromController();
}

void UMLHardwareBoardsScreen::HandleStartClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->PressStartCircuit(1);
	}
}

void UMLHardwareBoardsScreen::HandleStop1Changed(bool bIsChecked)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->SetStopCircuit(1, bIsChecked);
	}
}

void UMLHardwareBoardsScreen::HandleStop2Changed(bool bIsChecked)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->SetStopCircuit(2, bIsChecked);
	}
}

void UMLHardwareBoardsScreen::HandleStop3Changed(bool bIsChecked)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->SetStopCircuit(3, bIsChecked);
	}
}

void UMLHardwareBoardsScreen::HandleStop4Changed(bool bIsChecked)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->SetStopCircuit(4, bIsChecked);
	}
}

void UMLHardwareBoardsScreen::HandleStop5Changed(bool bIsChecked)
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->SetStopCircuit(5, bIsChecked);
	}
}
