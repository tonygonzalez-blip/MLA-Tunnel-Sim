// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "MicrologicTypes.h"
#include "MLPushButtonStation.generated.h"

class UBorder;
class UButton;
class UTextBlock;

/**
 * The physical LogicWash push-button station: 19 numbered buttons (1-5, 7-11,
 * 13-17, 19-22 — the physical layout skips 6/12/18), a KEY switch, and ENTER.
 *
 * Press flow (wash → retract(s) → ENTER): a Wash-type button sets the pending
 * wash, other programmable service buttons toggle membership in the pending
 * extras, and ENTER sends the assembled order. Momentarily On and Command
 * services execute immediately (Conveyor Start/Stop, Wetdown, Roller Up...).
 * With the key OFF, buttons 1-5 are locked out, as on the real box.
 * Buttons whose number has no service in the live config are disabled.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLPushButtonStation : public UMLControllerScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void RefreshFromController() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_4;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_5;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_7;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_8;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_9;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_10;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_11;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_13;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_14;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_15;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_16;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_17;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_19;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_20;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_21;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_22;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Key;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Enter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Display;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_KeyLed;

private:
	/** Wash selected for the pending order (0 = none yet). */
	int32 PendingWash = 0;

	/** Non-wash services toggled into the pending order, in press order. */
	TArray<int32> PendingExtras;

	bool bKeyOn = false;

	/** Button-number → widget map, rebuilt in NativeConstruct. */
	TArray<TPair<int32, UButton*>> NumberedButtons;

	void BuildButtonMap();
	void HandleNumberPressed(int32 Number);
	bool FindServiceForButton(int32 Number, FMLServiceConfig& OutService) const;
	void UpdateDisplay();
	void UpdateKeyLed();

	/** Enabled = mapped to a live service AND (key on OR number > 5). */
	void UpdateButtonEnablement();

	UFUNCTION()
	void HandleKeyPressed();

	UFUNCTION()
	void HandleEnterPressed();

	UFUNCTION()
	void HandleConfigChanged();

	// One handler per numbered button (dynamic delegates carry no payload).
	UFUNCTION()
	void HandleButton1();
	UFUNCTION()
	void HandleButton2();
	UFUNCTION()
	void HandleButton3();
	UFUNCTION()
	void HandleButton4();
	UFUNCTION()
	void HandleButton5();
	UFUNCTION()
	void HandleButton7();
	UFUNCTION()
	void HandleButton8();
	UFUNCTION()
	void HandleButton9();
	UFUNCTION()
	void HandleButton10();
	UFUNCTION()
	void HandleButton11();
	UFUNCTION()
	void HandleButton13();
	UFUNCTION()
	void HandleButton14();
	UFUNCTION()
	void HandleButton15();
	UFUNCTION()
	void HandleButton16();
	UFUNCTION()
	void HandleButton17();
	UFUNCTION()
	void HandleButton19();
	UFUNCTION()
	void HandleButton20();
	UFUNCTION()
	void HandleButton21();
	UFUNCTION()
	void HandleButton22();
};
