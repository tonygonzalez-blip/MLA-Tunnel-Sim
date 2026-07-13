// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLServicesScreen.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLServiceEditPanel.h"
#include "UI/MLServiceRow.h"

void UMLServicesScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_AddService)
	{
		Button_AddService->OnClicked.AddUniqueDynamic(this, &UMLServicesScreen::HandleAddServiceClicked);
	}

	if (Widget_EditPanel)
	{
		Widget_EditPanel->OnSaved.AddUniqueDynamic(this, &UMLServicesScreen::HandlePanelSaved);
		Widget_EditPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.AddUniqueDynamic(this, &UMLServicesScreen::HandleConfigChanged);
	}
}

void UMLServicesScreen::NativeDestruct()
{
	if (Widget_EditPanel)
	{
		Widget_EditPanel->OnSaved.RemoveDynamic(this, &UMLServicesScreen::HandlePanelSaved);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.RemoveDynamic(this, &UMLServicesScreen::HandleConfigChanged);
	}

	Super::NativeDestruct();
}

void UMLServicesScreen::RefreshFromController()
{
	if (!ScrollBox_Services)
	{
		return;
	}

	ScrollBox_Services->ClearChildren();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller || !ServiceRowClass)
	{
		return;
	}

	for (const FMLServiceConfig& Service : Controller->GetStagedConfig().Services)
	{
		if (UMLServiceRow* Row = CreateWidget<UMLServiceRow>(this, ServiceRowClass))
		{
			Row->OnEditRequested.AddUniqueDynamic(this, &UMLServicesScreen::HandleEditRequested);
			ScrollBox_Services->AddChild(Row);
			Row->SetService(Service);
		}
	}
}

void UMLServicesScreen::HandleEditRequested(int32 ServiceNumber)
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller || !Widget_EditPanel)
	{
		return;
	}

	for (const FMLServiceConfig& Service : Controller->GetStagedConfig().Services)
	{
		if (Service.ServiceNumber == ServiceNumber)
		{
			Widget_EditPanel->OpenForService(Service);
			return;
		}
	}
}

void UMLServicesScreen::HandleAddServiceClicked()
{
	if (!Widget_EditPanel)
	{
		return;
	}

	int32 SuggestedNumber = 1;
	if (const UMicrologicControllerSubsystem* Controller = GetController())
	{
		for (const FMLServiceConfig& Service : Controller->GetStagedConfig().Services)
		{
			SuggestedNumber = FMath::Max(SuggestedNumber, Service.ServiceNumber + 1);
		}
	}

	Widget_EditPanel->OpenForNew(SuggestedNumber);
}

void UMLServicesScreen::HandlePanelSaved()
{
	RefreshFromController();
}

void UMLServicesScreen::HandleConfigChanged()
{
	RefreshFromController();
}
