// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "Templates/SubclassOf.h"
#include "MLHardwareBoardsScreen.generated.h"

class UBorder;
class UButton;
class UCheckBox;
class UEditableTextBox;
class UMLRelaySwitchRow;
class UScrollBox;
class UUniformGridPanel;

/**
 * The virtual hardware view: input-board LEDs, the pulse LED, per-relay
 * manual switches, the start/stop board (five NC stop circuits + a momentary
 * start button), and the starter/horn indicator LEDs. Trainees can open a
 * stop circuit here and watch the conveyor die.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLHardwareBoardsScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

public:
	/** Row widget blueprint (parent class UMLRelaySwitchRow). */
	UPROPERTY(EditAnywhere, Category = "Micrologic|UI")
	TSubclassOf<UMLRelaySwitchRow> RelaySwitchRowClass;

	/** The PIN typed into the shared PIN box (relay-override PIN). */
	UFUNCTION(BlueprintPure, Category = "Micrologic|UI")
	FString GetPin() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void RefreshFromController() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> UniformGridPanel_InputLeds;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_PulseLed;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_RelaySwitches;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Pin;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Stop1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Stop2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Stop3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Stop4;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Stop5;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Start;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_StarterLed;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_HornLed;

private:
	/** Number of LED cells on the virtual input board. */
	static constexpr int32 NumInputLeds = 16;

	/** LED borders keyed by input channel. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBorder>> InputLedsByChannel;

	void RebuildInputLeds();
	void RebuildRelaySwitches();
	void SyncStopCheckBoxes();

	UFUNCTION()
	void HandleInputStateChanged(int32 Channel, bool bOn);

	UFUNCTION()
	void HandleConfigChanged();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleStop1Changed(bool bIsChecked);
	UFUNCTION()
	void HandleStop2Changed(bool bIsChecked);
	UFUNCTION()
	void HandleStop3Changed(bool bIsChecked);
	UFUNCTION()
	void HandleStop4Changed(bool bIsChecked);
	UFUNCTION()
	void HandleStop5Changed(bool bIsChecked);
};
