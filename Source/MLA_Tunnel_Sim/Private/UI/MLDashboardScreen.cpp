// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLDashboardScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLUiText.h"

namespace
{
	constexpr int32 GridColumns = 8;
	constexpr float PulseLitSeconds = 0.15f;
	constexpr float CarListRefreshPeriod = 0.25f;
}

void UMLDashboardScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_ConveyorStart)
	{
		Button_ConveyorStart->OnClicked.AddUniqueDynamic(this, &UMLDashboardScreen::HandleStartClicked);
	}
	if (Button_ConveyorStop)
	{
		Button_ConveyorStop->OnClicked.AddUniqueDynamic(this, &UMLDashboardScreen::HandleStopClicked);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.AddUniqueDynamic(this, &UMLDashboardScreen::HandleRelayStateChanged);
		Controller->OnInputStateChanged.AddUniqueDynamic(this, &UMLDashboardScreen::HandleInputStateChanged);
		Controller->OnConveyorStateChanged.AddUniqueDynamic(this, &UMLDashboardScreen::HandleConveyorStateChanged);
		Controller->OnQueueChanged.AddUniqueDynamic(this, &UMLDashboardScreen::HandleQueueChanged);
		Controller->OnPulse.AddUniqueDynamic(this, &UMLDashboardScreen::HandlePulse);
	}
}

void UMLDashboardScreen::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnRelayStateChanged.RemoveDynamic(this, &UMLDashboardScreen::HandleRelayStateChanged);
		Controller->OnInputStateChanged.RemoveDynamic(this, &UMLDashboardScreen::HandleInputStateChanged);
		Controller->OnConveyorStateChanged.RemoveDynamic(this, &UMLDashboardScreen::HandleConveyorStateChanged);
		Controller->OnQueueChanged.RemoveDynamic(this, &UMLDashboardScreen::HandleQueueChanged);
		Controller->OnPulse.RemoveDynamic(this, &UMLDashboardScreen::HandlePulse);
	}

	Super::NativeDestruct();
}

void UMLDashboardScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Pulse LED dims shortly after each pulse (HandlePulse lights it).
	if (Border_PulseLed && Controller->GetTimeSinceLastPulse() >= PulseLitSeconds)
	{
		Border_PulseLed->SetBrushColor(MLUi::OffColor);
	}

	// Cars move continuously with no per-move event; rebuild the (cheap,
	// text-only) list on a small fixed period.
	CarListRefreshAccumulator += InDeltaTime;
	if (CarListRefreshAccumulator >= CarListRefreshPeriod)
	{
		CarListRefreshAccumulator = 0.f;
		RebuildCarsList();
	}
}

void UMLDashboardScreen::RefreshFromController()
{
	RefreshHeaderTexts();
	RebuildRelayGrid();
	RebuildInputGrid();
	RebuildCarsList();
	RebuildQueueList();
}

void UMLDashboardScreen::RefreshHeaderTexts()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	if (TextBlock_ConveyorState)
	{
		TextBlock_ConveyorState->SetText(FText::FromString(MLUi::ConveyorStateText(Controller->GetConveyorState())));
		TextBlock_ConveyorState->SetColorAndOpacity(FSlateColor(Controller->IsConveyorRunning() ? MLUi::OnColor : MLUi::FaultColor));
	}

	if (TextBlock_StopReason)
	{
		// Only meaningful while stopped; blank while running.
		const FString Reason = Controller->IsConveyorRunning()
			? FString()
			: MLUi::StopReasonText(Controller->GetLastStopReason());
		TextBlock_StopReason->SetText(FText::FromString(Reason));
	}

	if (TextBlock_QueueCount)
	{
		TextBlock_QueueCount->SetText(FText::AsNumber(Controller->GetQueue().Num()));
	}
}

UBorder* UMLDashboardScreen::MakeLedCell(UUniformGridPanel* Grid, int32 CellIndex, int32 Number, const FString& Tooltip, bool bOn)
{
	if (!Grid || !WidgetTree)
	{
		return nullptr;
	}

	UBorder* Led = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (!Led)
	{
		return nullptr;
	}
	Led->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
	Led->SetPadding(FMargin(6.f, 3.f));
	if (!Tooltip.IsEmpty())
	{
		Led->SetToolTipText(FText::FromString(Tooltip));
	}

	if (UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
	{
		Label->SetText(FText::AsNumber(Number));
		Label->SetJustification(ETextJustify::Center);
		Led->SetContent(Label);
	}

	if (UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Led, CellIndex / GridColumns, CellIndex % GridColumns))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}
	return Led;
}

void UMLDashboardScreen::RebuildRelayGrid()
{
	if (!UniformGridPanel_Relays)
	{
		return;
	}

	UniformGridPanel_Relays->ClearChildren();
	RelayCellsByNumber.Empty();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Descriptions for tooltips come from the live config.
	TMap<int32, FString> Descriptions;
	for (const FMLRelayConfig& Relay : Controller->GetLiveConfig().Relays)
	{
		Descriptions.Add(Relay.RelayNumber, Relay.Description);
	}

	int32 CellIndex = 0;
	for (const FMLRelayRuntime& Relay : Controller->GetRelayStates())
	{
		const FString* Description = Descriptions.Find(Relay.RelayNumber);
		UBorder* Led = MakeLedCell(UniformGridPanel_Relays, CellIndex++, Relay.RelayNumber,
			Description ? *Description : FString(), Relay.bPhysicalOn);
		if (Led)
		{
			RelayCellsByNumber.Add(Relay.RelayNumber, Led);
		}
	}
}

void UMLDashboardScreen::RebuildInputGrid()
{
	if (!UniformGridPanel_Inputs)
	{
		return;
	}

	UniformGridPanel_Inputs->ClearChildren();
	InputCellsByChannel.Empty();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	TMap<int32, FString> Descriptions;
	for (const FMLInputConfig& Input : Controller->GetLiveConfig().Inputs)
	{
		Descriptions.Add(Input.Channel, Input.Description);
	}

	int32 CellIndex = 0;
	for (const FMLInputRuntime& Input : Controller->GetInputStates())
	{
		const FString* Description = Descriptions.Find(Input.Channel);
		UBorder* Led = MakeLedCell(UniformGridPanel_Inputs, CellIndex++, Input.Channel,
			Description ? *Description : FString(), Input.bCommitted);
		if (Led)
		{
			InputCellsByChannel.Add(Input.Channel, Led);
		}
	}
}

void UMLDashboardScreen::RebuildCarsList()
{
	if (!ScrollBox_Cars || !WidgetTree)
	{
		return;
	}

	ScrollBox_Cars->ClearChildren();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	for (const FMLCarState& Car : Controller->GetCars())
	{
		if (UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			const FString Services = MLUi::IntArrayToCsv(Car.ServiceNumbers);
			Row->SetText(FText::FromString(FString::Printf(
				TEXT("Car %d — %.1f ft @ %.1f ft — services: %s"),
				Car.CarId, Car.LengthFeet, Car.FrontPositionFeet, *Services)));
			ScrollBox_Cars->AddChild(Row);
		}
	}
}

void UMLDashboardScreen::RebuildQueueList()
{
	if (!ScrollBox_Queue || !WidgetTree)
	{
		return;
	}

	ScrollBox_Queue->ClearChildren();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	for (const FMLOrder& Order : Controller->GetQueue())
	{
		if (UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			const FString Services = MLUi::IntArrayToCsv(Order.ServiceNumbers);
			Row->SetText(FText::FromString(FString::Printf(
				TEXT("Order %d — services: %s"), Order.OrderId, *Services)));
			ScrollBox_Queue->AddChild(Row);
		}
	}
}

void UMLDashboardScreen::HandleRelayStateChanged(int32 RelayNumber, bool bOn)
{
	if (const TObjectPtr<UBorder>* Led = RelayCellsByNumber.Find(RelayNumber))
	{
		if (*Led)
		{
			(*Led)->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
		}
	}
}

void UMLDashboardScreen::HandleInputStateChanged(int32 Channel, bool bOn)
{
	if (const TObjectPtr<UBorder>* Led = InputCellsByChannel.Find(Channel))
	{
		if (*Led)
		{
			(*Led)->SetBrushColor(bOn ? MLUi::OnColor : MLUi::OffColor);
		}
	}
}

void UMLDashboardScreen::HandleConveyorStateChanged(EMLConveyorState NewState)
{
	RefreshHeaderTexts();
}

void UMLDashboardScreen::HandleQueueChanged()
{
	RefreshHeaderTexts();
	RebuildQueueList();
}

void UMLDashboardScreen::HandlePulse()
{
	if (Border_PulseLed)
	{
		Border_PulseLed->SetBrushColor(MLUi::OnColor);
	}
}

void UMLDashboardScreen::HandleStartClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		const int32 Service = Controller->GetLiveConfig().Conveyor.OnActivationService;
		if (Service > 0)
		{
			Controller->ExecuteService(Service);
		}
	}
}

void UMLDashboardScreen::HandleStopClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		const int32 Service = Controller->GetLiveConfig().Conveyor.ShutOffService;
		if (Service > 0)
		{
			Controller->ExecuteService(Service);
		}
	}
}
