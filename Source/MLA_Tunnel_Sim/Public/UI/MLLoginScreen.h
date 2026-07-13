// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "MLLoginScreen.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMLLoginSucceeded);

/**
 * The web-UI login page (real controller: manager / manager01).
 * Validates against UMicrologicControllerSubsystem::ValidateLogin.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLLoginScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

public:
	/** Fired when ValidateLogin accepts the entered credentials. */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLLoginSucceeded OnLoginSucceeded;

protected:
	virtual void NativeConstruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Username;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Password;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Login;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Error;

private:
	UFUNCTION()
	void HandleLoginClicked();
};
