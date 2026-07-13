// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLSettingsScreen.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

void UMLSettingsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_TabCommunications)
	{
		Button_TabCommunications->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabCommunications);
	}
	if (Button_TabConveyor)
	{
		Button_TabConveyor->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabConveyor);
	}
	if (Button_TabAntiCollision)
	{
		Button_TabAntiCollision->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabAntiCollision);
	}
	if (Button_TabRollerDefaults)
	{
		Button_TabRollerDefaults->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabRollerDefaults);
	}
	if (Button_TabSecurity)
	{
		Button_TabSecurity->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabSecurity);
	}
	if (Button_TabBackupRestore)
	{
		Button_TabBackupRestore->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabBackupRestore);
	}
	if (Button_TabSonar)
	{
		Button_TabSonar->OnClicked.AddUniqueDynamic(this, &UMLSettingsScreen::HandleTabSonar);
	}
}

void UMLSettingsScreen::SetTab(int32 Index)
{
	if (WidgetSwitcher_Tabs)
	{
		WidgetSwitcher_Tabs->SetActiveWidgetIndex(Index);
	}
}

void UMLSettingsScreen::HandleTabCommunications() { SetTab(0); }
void UMLSettingsScreen::HandleTabConveyor()       { SetTab(1); }
void UMLSettingsScreen::HandleTabAntiCollision()  { SetTab(2); }
void UMLSettingsScreen::HandleTabRollerDefaults() { SetTab(3); }
void UMLSettingsScreen::HandleTabSecurity()       { SetTab(4); }
void UMLSettingsScreen::HandleTabBackupRestore()  { SetTab(5); }
void UMLSettingsScreen::HandleTabSonar()          { SetTab(6); }
