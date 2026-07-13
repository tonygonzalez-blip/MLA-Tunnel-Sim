// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLLoginScreen.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "MicrologicControllerSubsystem.h"

void UMLLoginScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Login)
	{
		Button_Login->OnClicked.AddUniqueDynamic(this, &UMLLoginScreen::HandleLoginClicked);
	}

	if (TextBlock_Error)
	{
		TextBlock_Error->SetText(FText::GetEmpty());
	}
}

void UMLLoginScreen::HandleLoginClicked()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FString Username = EditableTextBox_Username ? EditableTextBox_Username->GetText().ToString() : FString();
	const FString Password = EditableTextBox_Password ? EditableTextBox_Password->GetText().ToString() : FString();

	if (Controller->ValidateLogin(Username, Password))
	{
		if (TextBlock_Error)
		{
			TextBlock_Error->SetText(FText::GetEmpty());
		}
		if (EditableTextBox_Password)
		{
			EditableTextBox_Password->SetText(FText::GetEmpty());
		}
		OnLoginSucceeded.Broadcast();
	}
	else if (TextBlock_Error)
	{
		TextBlock_Error->SetText(FText::FromString(TEXT("Invalid username or password.")));
	}
}
