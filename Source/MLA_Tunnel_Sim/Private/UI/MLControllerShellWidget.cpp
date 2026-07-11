// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLControllerShellWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLLoginScreen.h"

void UMLControllerShellWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TextBlock_AddressBar)
	{
		TextBlock_AddressBar->SetText(FText::FromString(TEXT("http://10.0.1.90/")));
	}

	if (WidgetSwitcher_Root)
	{
		WidgetSwitcher_Root->SetActiveWidgetIndex(bLoggedIn ? 1 : 0);
	}

	if (Border_LockOverlay)
	{
		Border_LockOverlay->SetVisibility(bLocked ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (Widget_Login)
	{
		Widget_Login->OnLoginSucceeded.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleLoginSucceeded);
	}

	if (Button_NavDashboard)
	{
		Button_NavDashboard->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleNavDashboard);
	}
	if (Button_NavSettings)
	{
		Button_NavSettings->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleNavSettings);
	}
	if (Button_NavInputs)
	{
		Button_NavInputs->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleNavInputs);
	}
	if (Button_NavRelays)
	{
		Button_NavRelays->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleNavRelays);
	}
	if (Button_NavServices)
	{
		Button_NavServices->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleNavServices);
	}
	if (Button_NavBoards)
	{
		Button_NavBoards->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleNavBoards);
	}
	if (Button_Unlock)
	{
		Button_Unlock->OnClicked.AddUniqueDynamic(this, &UMLControllerShellWidget::HandleUnlockClicked);
	}
}

void UMLControllerShellWidget::NativeDestruct()
{
	if (Widget_Login)
	{
		Widget_Login->OnLoginSucceeded.RemoveDynamic(this, &UMLControllerShellWidget::HandleLoginSucceeded);
	}

	Super::NativeDestruct();
}

void UMLControllerShellWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Idle auto-lock only applies once someone is logged in and not already
	// staring at the lock overlay.
	if (bLoggedIn && !bLocked)
	{
		IdleSeconds += InDeltaTime;
		if (IdleSeconds >= AutoLockSeconds)
		{
			LockUi();
		}
	}
}

void UMLControllerShellWidget::LockUi()
{
	bLocked = true;

	if (Border_LockOverlay)
	{
		Border_LockOverlay->SetVisibility(ESlateVisibility::Visible);
	}
	if (EditableTextBox_UnlockPassword)
	{
		EditableTextBox_UnlockPassword->SetText(FText::GetEmpty());
	}
}

void UMLControllerShellWidget::SetScreen(int32 Index)
{
	IdleSeconds = 0.f;

	if (WidgetSwitcher_Screens)
	{
		WidgetSwitcher_Screens->SetActiveWidgetIndex(Index);
	}
}

void UMLControllerShellWidget::HandleLoginSucceeded()
{
	bLoggedIn = true;
	IdleSeconds = 0.f;

	if (WidgetSwitcher_Root)
	{
		WidgetSwitcher_Root->SetActiveWidgetIndex(1);
	}
}

void UMLControllerShellWidget::HandleNavDashboard() { SetScreen(0); }
void UMLControllerShellWidget::HandleNavSettings()  { SetScreen(1); }
void UMLControllerShellWidget::HandleNavInputs()    { SetScreen(2); }
void UMLControllerShellWidget::HandleNavRelays()    { SetScreen(3); }
void UMLControllerShellWidget::HandleNavServices()  { SetScreen(4); }
void UMLControllerShellWidget::HandleNavBoards()    { SetScreen(5); }

void UMLControllerShellWidget::HandleUnlockClicked()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FString Password = EditableTextBox_UnlockPassword ? EditableTextBox_UnlockPassword->GetText().ToString() : FString();

	if (Controller->ValidateUiPassword(Password))
	{
		bLocked = false;
		IdleSeconds = 0.f;

		if (Border_LockOverlay)
		{
			Border_LockOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EditableTextBox_UnlockPassword)
	{
		EditableTextBox_UnlockPassword->SetText(FText::GetEmpty());
	}
}
