// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLRelaysScreen.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLRelayEditPanel.h"
#include "UI/MLRelayRow.h"

void UMLRelaysScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_AddRelay)
	{
		Button_AddRelay->OnClicked.AddUniqueDynamic(this, &UMLRelaysScreen::HandleAddRelayClicked);
	}

	if (Widget_EditPanel)
	{
		Widget_EditPanel->OnSaved.AddUniqueDynamic(this, &UMLRelaysScreen::HandlePanelSaved);
		Widget_EditPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.AddUniqueDynamic(this, &UMLRelaysScreen::HandleConfigChanged);
	}
}

void UMLRelaysScreen::NativeDestruct()
{
	if (Widget_EditPanel)
	{
		Widget_EditPanel->OnSaved.RemoveDynamic(this, &UMLRelaysScreen::HandlePanelSaved);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.RemoveDynamic(this, &UMLRelaysScreen::HandleConfigChanged);
	}

	Super::NativeDestruct();
}

void UMLRelaysScreen::RefreshFromController()
{
	if (!ScrollBox_Relays)
	{
		return;
	}

	ScrollBox_Relays->ClearChildren();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller || !RelayRowClass)
	{
		return;
	}

	for (const FMLRelayConfig& Relay : Controller->GetStagedConfig().Relays)
	{
		if (UMLRelayRow* Row = CreateWidget<UMLRelayRow>(this, RelayRowClass))
		{
			Row->OnEditRequested.AddUniqueDynamic(this, &UMLRelaysScreen::HandleEditRequested);
			ScrollBox_Relays->AddChild(Row);
			Row->SetRelay(Relay);
		}
	}
}

void UMLRelaysScreen::HandleEditRequested(int32 RelayNumber)
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller || !Widget_EditPanel)
	{
		return;
	}

	for (const FMLRelayConfig& Relay : Controller->GetStagedConfig().Relays)
	{
		if (Relay.RelayNumber == RelayNumber)
		{
			Widget_EditPanel->OpenForRelay(Relay);
			return;
		}
	}
}

void UMLRelaysScreen::HandleAddRelayClicked()
{
	if (!Widget_EditPanel)
	{
		return;
	}

	int32 SuggestedNumber = 1;
	if (const UMicrologicControllerSubsystem* Controller = GetController())
	{
		for (const FMLRelayConfig& Relay : Controller->GetStagedConfig().Relays)
		{
			SuggestedNumber = FMath::Max(SuggestedNumber, Relay.RelayNumber + 1);
		}
	}

	Widget_EditPanel->OpenForNew(SuggestedNumber);
}

void UMLRelaysScreen::HandlePanelSaved()
{
	RefreshFromController();
}

void UMLRelaysScreen::HandleConfigChanged()
{
	RefreshFromController();
}
