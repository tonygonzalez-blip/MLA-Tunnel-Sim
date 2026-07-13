// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLInputsScreen.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLInputRow.h"

void UMLInputsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Save)
	{
		Button_Save->OnClicked.AddUniqueDynamic(this, &UMLInputsScreen::HandleSaveClicked);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		// Commit / Restore / Reset replace the staged config wholesale.
		Controller->OnConfigChanged.AddUniqueDynamic(this, &UMLInputsScreen::HandleConfigChanged);
	}
}

void UMLInputsScreen::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.RemoveDynamic(this, &UMLInputsScreen::HandleConfigChanged);
	}

	Super::NativeDestruct();
}

void UMLInputsScreen::RefreshFromController()
{
	if (!ScrollBox_Inputs)
	{
		return;
	}

	ScrollBox_Inputs->ClearChildren();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller || !InputRowClass)
	{
		return;
	}

	for (const FMLInputConfig& Input : Controller->GetStagedConfig().Inputs)
	{
		if (UMLInputRow* Row = CreateWidget<UMLInputRow>(this, InputRowClass))
		{
			ScrollBox_Inputs->AddChild(Row);
			Row->SetInput(Input);
		}
	}
}

void UMLInputsScreen::HandleSaveClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->Commit();
	}
}

void UMLInputsScreen::HandleConfigChanged()
{
	RefreshFromController();
}
