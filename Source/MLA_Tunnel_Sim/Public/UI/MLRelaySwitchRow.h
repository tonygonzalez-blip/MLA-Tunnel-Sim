// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "MLRelaySwitchRow.generated.h"

class UBorder;
class UButton;
class UMLHardwareBoardsScreen;
class UTextBlock;

/**
 * One relay on the virtual output board: number + description, the physical
 * 3-position AUTO/ON/OFF manual switch, and the relay's physical-state LED.
 * The PIN (when relay overrides require one) comes from the owning
 * UMLHardwareBoardsScreen's shared PIN box.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLRelaySwitchRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetRelay(int32 InRelayNumber, const FString& InDescription);

	/** The screen whose GetPin() supplies the override PIN. */
	void SetPinSource(UMLHardwareBoardsScreen* InPinSource);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Relay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Auto;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_On;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Off;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Led;

private:
	int32 RelayNumber = 0;

	TWeakObjectPtr<UMLHardwareBoardsScreen> PinSource;

	class UMicrologicControllerSubsystem* GetController() const;
	void RequestOverride(EMLRelayOverride Override);

	/** Reflect the ACTUAL switch position (disable the active position's button). */
	void UpdateSwitchUi();
	void UpdateLed(bool bOn);

	UFUNCTION()
	void HandleAuto();

	UFUNCTION()
	void HandleOn();

	UFUNCTION()
	void HandleOff();

	UFUNCTION()
	void HandleRelayStateChanged(int32 InRelayNumber, bool bOn);
};
