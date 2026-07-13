// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "MLRelayRow.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLRelayEditRequested, int32, RelayNumber);

/**
 * One row on the Relays page: summary text, live physical-state LED, an Edit
 * button (screen opens the edit panel), and the 3-position AUTO/ON/OFF
 * override switch with its PIN box.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLRelayRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load this row from a staged relay config. */
	void SetRelay(const FMLRelayConfig& InConfig);

	/** Fired when the row's Edit button is clicked. */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLRelayEditRequested OnEditRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Number;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Type;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Default;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Led;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Edit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SwitchAuto;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SwitchOn;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SwitchOff;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Pin;

private:
	FMLRelayConfig Config;

	class UMicrologicControllerSubsystem* GetController() const;
	void RequestOverride(EMLRelayOverride Override);

	/** Reflect the ACTUAL switch position (disable the active position's button). */
	void UpdateSwitchUi();
	void UpdateLed(bool bOn);

	UFUNCTION()
	void HandleEditClicked();

	UFUNCTION()
	void HandleSwitchAuto();

	UFUNCTION()
	void HandleSwitchOn();

	UFUNCTION()
	void HandleSwitchOff();

	UFUNCTION()
	void HandleRelayStateChanged(int32 RelayNumber, bool bOn);
};
