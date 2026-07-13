// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLPushButtonStation.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLUiText.h"

void UMLPushButtonStation::NativeConstruct()
{
	Super::NativeConstruct();

	BuildButtonMap();

	// Bind numbered buttons. Dynamic delegates cannot carry a payload, so
	// each button gets its own thunk into HandleNumberPressed.
	if (Button_1)  { Button_1->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton1); }
	if (Button_2)  { Button_2->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton2); }
	if (Button_3)  { Button_3->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton3); }
	if (Button_4)  { Button_4->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton4); }
	if (Button_5)  { Button_5->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton5); }
	if (Button_7)  { Button_7->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton7); }
	if (Button_8)  { Button_8->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton8); }
	if (Button_9)  { Button_9->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton9); }
	if (Button_10) { Button_10->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton10); }
	if (Button_11) { Button_11->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton11); }
	if (Button_13) { Button_13->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton13); }
	if (Button_14) { Button_14->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton14); }
	if (Button_15) { Button_15->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton15); }
	if (Button_16) { Button_16->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton16); }
	if (Button_17) { Button_17->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton17); }
	if (Button_19) { Button_19->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton19); }
	if (Button_20) { Button_20->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton20); }
	if (Button_21) { Button_21->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton21); }
	if (Button_22) { Button_22->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleButton22); }

	if (Button_Key)
	{
		Button_Key->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleKeyPressed);
	}
	if (Button_Enter)
	{
		Button_Enter->OnClicked.AddUniqueDynamic(this, &UMLPushButtonStation::HandleEnterPressed);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.AddUniqueDynamic(this, &UMLPushButtonStation::HandleConfigChanged);
	}

	UpdateDisplay();
	UpdateKeyLed();
	UpdateButtonEnablement();
}

void UMLPushButtonStation::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.RemoveDynamic(this, &UMLPushButtonStation::HandleConfigChanged);
	}

	Super::NativeDestruct();
}

void UMLPushButtonStation::RefreshFromController()
{
	BuildButtonMap();
	UpdateButtonEnablement();
}

void UMLPushButtonStation::BuildButtonMap()
{
	NumberedButtons.Reset();
	NumberedButtons.Add({ 1, Button_1 });
	NumberedButtons.Add({ 2, Button_2 });
	NumberedButtons.Add({ 3, Button_3 });
	NumberedButtons.Add({ 4, Button_4 });
	NumberedButtons.Add({ 5, Button_5 });
	NumberedButtons.Add({ 7, Button_7 });
	NumberedButtons.Add({ 8, Button_8 });
	NumberedButtons.Add({ 9, Button_9 });
	NumberedButtons.Add({ 10, Button_10 });
	NumberedButtons.Add({ 11, Button_11 });
	NumberedButtons.Add({ 13, Button_13 });
	NumberedButtons.Add({ 14, Button_14 });
	NumberedButtons.Add({ 15, Button_15 });
	NumberedButtons.Add({ 16, Button_16 });
	NumberedButtons.Add({ 17, Button_17 });
	NumberedButtons.Add({ 19, Button_19 });
	NumberedButtons.Add({ 20, Button_20 });
	NumberedButtons.Add({ 21, Button_21 });
	NumberedButtons.Add({ 22, Button_22 });
}

bool UMLPushButtonStation::FindServiceForButton(int32 Number, FMLServiceConfig& OutService) const
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return false;
	}

	// Button N fires service N — the real station's panel is labeled to match
	// the live service table.
	for (const FMLServiceConfig& Service : Controller->GetLiveConfig().Services)
	{
		if (Service.ServiceNumber == Number)
		{
			OutService = Service;
			return true;
		}
	}
	return false;
}

void UMLPushButtonStation::HandleNumberPressed(int32 Number)
{
	// Key lockout: with the key OFF, buttons 1-5 are dead (they are also
	// disabled visually, but guard anyway).
	if (!bKeyOn && Number <= 5)
	{
		return;
	}

	FMLServiceConfig Service;
	if (!FindServiceForButton(Number, Service))
	{
		return;
	}

	switch (Service.Type)
	{
	case EMLServiceType::Wash:
		// A wash starts (or replaces) the pending order.
		PendingWash = Number;
		break;

	case EMLServiceType::MomentarilyOn:
	case EMLServiceType::Command:
		// Conveyor Start/Stop, Wetdown, Roller Up... fire immediately.
		if (UMicrologicControllerSubsystem* Controller = GetController())
		{
			Controller->ExecuteService(Number);
		}
		return;

	default:
		// Retracts / add-ons toggle membership in the pending order.
		if (PendingExtras.Contains(Number))
		{
			PendingExtras.Remove(Number);
		}
		else
		{
			PendingExtras.Add(Number);
		}
		break;
	}

	UpdateDisplay();
}

void UMLPushButtonStation::UpdateDisplay()
{
	if (!TextBlock_Display)
	{
		return;
	}

	TArray<FString> Parts;
	if (PendingWash > 0)
	{
		Parts.Add(FString::FromInt(PendingWash));
	}
	for (int32 Extra : PendingExtras)
	{
		Parts.Add(FString::FromInt(Extra));
	}

	TextBlock_Display->SetText(FText::FromString(FString::Join(Parts, TEXT(" + "))));
}

void UMLPushButtonStation::UpdateKeyLed()
{
	if (Border_KeyLed)
	{
		Border_KeyLed->SetBrushColor(bKeyOn ? MLUi::OnColor : MLUi::OffColor);
	}
}

void UMLPushButtonStation::UpdateButtonEnablement()
{
	FMLServiceConfig Unused;
	for (const TPair<int32, UButton*>& Entry : NumberedButtons)
	{
		if (!Entry.Value)
		{
			continue;
		}

		const bool bMapped = FindServiceForButton(Entry.Key, Unused);
		const bool bKeyAllows = bKeyOn || Entry.Key > 5;
		Entry.Value->SetIsEnabled(bMapped && bKeyAllows);
	}
}

void UMLPushButtonStation::HandleKeyPressed()
{
	bKeyOn = !bKeyOn;
	UpdateKeyLed();
	UpdateButtonEnablement();
}

void UMLPushButtonStation::HandleEnterPressed()
{
	TArray<int32> Order;
	if (PendingWash > 0)
	{
		Order.Add(PendingWash);
	}
	Order.Append(PendingExtras);

	if (Order.Num() == 0)
	{
		return;
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->SendOrder(Order);
	}

	PendingWash = 0;
	PendingExtras.Reset();
	UpdateDisplay();
}

void UMLPushButtonStation::HandleConfigChanged()
{
	UpdateButtonEnablement();
}

void UMLPushButtonStation::HandleButton1()  { HandleNumberPressed(1); }
void UMLPushButtonStation::HandleButton2()  { HandleNumberPressed(2); }
void UMLPushButtonStation::HandleButton3()  { HandleNumberPressed(3); }
void UMLPushButtonStation::HandleButton4()  { HandleNumberPressed(4); }
void UMLPushButtonStation::HandleButton5()  { HandleNumberPressed(5); }
void UMLPushButtonStation::HandleButton7()  { HandleNumberPressed(7); }
void UMLPushButtonStation::HandleButton8()  { HandleNumberPressed(8); }
void UMLPushButtonStation::HandleButton9()  { HandleNumberPressed(9); }
void UMLPushButtonStation::HandleButton10() { HandleNumberPressed(10); }
void UMLPushButtonStation::HandleButton11() { HandleNumberPressed(11); }
void UMLPushButtonStation::HandleButton13() { HandleNumberPressed(13); }
void UMLPushButtonStation::HandleButton14() { HandleNumberPressed(14); }
void UMLPushButtonStation::HandleButton15() { HandleNumberPressed(15); }
void UMLPushButtonStation::HandleButton16() { HandleNumberPressed(16); }
void UMLPushButtonStation::HandleButton17() { HandleNumberPressed(17); }
void UMLPushButtonStation::HandleButton19() { HandleNumberPressed(19); }
void UMLPushButtonStation::HandleButton20() { HandleNumberPressed(20); }
void UMLPushButtonStation::HandleButton21() { HandleNumberPressed(21); }
void UMLPushButtonStation::HandleButton22() { HandleNumberPressed(22); }
