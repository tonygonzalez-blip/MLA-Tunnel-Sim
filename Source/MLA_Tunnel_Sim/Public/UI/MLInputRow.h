// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "Types/SlateEnums.h"
#include "MLInputRow.generated.h"

class UBorder;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class USpinBox;
class UTextBlock;

/**
 * One row on the Inputs page. Edits push straight into the staged config via
 * UMicrologicControllerSubsystem::UpsertStagedInput; the LED shows the LIVE
 * committed state of the channel.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLInputRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load this row from a staged input config. */
	void SetInput(const FMLInputConfig& InConfig);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Channel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_Type;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Inverted;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Debounce;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_TriggerService;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Led;

private:
	/** The row's working copy of the staged input. */
	FMLInputConfig Config;

	class UMicrologicControllerSubsystem* GetController() const;
	void PushToStaged();
	void UpdateLed(bool bOn);

	UFUNCTION()
	void HandleDescriptionCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleInvertedChanged(bool bIsChecked);

	UFUNCTION()
	void HandleDebounceCommitted(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleTriggerServiceCommitted(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleInputStateChanged(int32 Channel, bool bOn);
};
