// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "Templates/SubclassOf.h"
#include "MLRelayEditPanel.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UMLModifierRow;
class UScrollBox;
class USpinBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMLEditPanelSaved);

/**
 * The relay edit form (Relays page). Edits a working copy of one
 * FMLRelayConfig — including its function and modifier list — and pushes it
 * into the staged config with UpsertStagedRelay on Save.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLRelayEditPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Open the panel editing an existing staged relay. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|UI")
	void OpenForRelay(const FMLRelayConfig& InRelay);

	/** Open the panel for a brand-new relay with a suggested number. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|UI")
	void OpenForNew(int32 SuggestedNumber);

	/** Fired after Save writes the working copy into the staged config. */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLEditPanelSaved OnSaved;

	/** Modifier-row widget blueprint (parent class UMLModifierRow). */
	UPROPERTY(EditAnywhere, Category = "Micrologic|UI")
	TSubclassOf<UMLModifierRow> ModifierRowClass;

protected:
	virtual void NativeConstruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_RelayNumber;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Active;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Default;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_Type;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_InactivityCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_InterlockStart;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_InterlockStop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_LookBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_FunctionType;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_DevicePosition;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_TurnOnFeet;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_TurnOnRef;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_TurnOffFeet;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_TurnOffRef;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Modifiers;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_AddModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Save;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Cancel;

private:
	/** The working copy being edited (Working.Function.Modifiers is the modifier list). */
	FMLRelayConfig Working;

	class UMicrologicControllerSubsystem* GetController() const;

	void LoadWorkingToUi();

	/** Read every bound field back into Working (unbound fields keep their value). */
	void PullUiToWorking();

	void RebuildModifierRows();
	void Open();

	UFUNCTION()
	void HandleModifierChanged(int32 ModifierIndex, FMLModifierConfig Modifier);

	UFUNCTION()
	void HandleModifierRemove(int32 ModifierIndex);

	UFUNCTION()
	void HandleAddModifierClicked();

	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleCancelClicked();
};
