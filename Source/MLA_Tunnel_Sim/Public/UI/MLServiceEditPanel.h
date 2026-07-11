// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "MLServiceEditPanel.generated.h"

class UButton;
class UComboBoxString;
class UEditableTextBox;
class USpinBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMLServicePanelSaved);

/**
 * The service edit form (Services page). Relay and macro-service lists are
 * edited as CSV text ("9,10,18"); non-numeric entries are ignored on save.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLServiceEditPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Open the panel editing an existing staged service. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|UI")
	void OpenForService(const FMLServiceConfig& InService);

	/** Open the panel for a brand-new service with a suggested number. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|UI")
	void OpenForNew(int32 SuggestedNumber);

	/** Fired after Save or Delete changes the staged config. */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLServicePanelSaved OnSaved;

protected:
	virtual void NativeConstruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_ServiceNumber;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_Type;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Relays;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Time;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Delay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_MacroServices;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_Command;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Save;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Cancel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Delete;

private:
	FMLServiceConfig Working;

	class UMicrologicControllerSubsystem* GetController() const;
	void LoadWorkingToUi();
	void PullUiToWorking();
	void Open();

	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleDeleteClicked();
};
