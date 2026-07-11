// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "MicrologicTypes.h"
#include "MLDashboardScreen.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UUniformGridPanel;

/**
 * Dashboard: conveyor state / stop reason / queue count, live relay + input
 * LED grids (built programmatically, 8 per row), cars-in-tunnel and queue
 * lists, and the pulse LED.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLDashboardScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void RefreshFromController() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_ConveyorState;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_StopReason;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_QueueCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConveyorStart;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConveyorStop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> UniformGridPanel_Relays;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> UniformGridPanel_Inputs;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Cars;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Queue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_PulseLed;

private:
	/** LED cells built into UniformGridPanel_Relays, keyed by relay number. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBorder>> RelayCellsByNumber;

	/** LED cells built into UniformGridPanel_Inputs, keyed by channel. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBorder>> InputCellsByChannel;

	/** Car positions move every pulse; throttle list rebuilds to this period. */
	float CarListRefreshAccumulator = 0.f;

	void RefreshHeaderTexts();
	void RebuildRelayGrid();
	void RebuildInputGrid();
	void RebuildCarsList();
	void RebuildQueueList();

	/** Construct one LED border + number label into a grid. */
	UBorder* MakeLedCell(UUniformGridPanel* Grid, int32 CellIndex, int32 Number, const FString& Tooltip, bool bOn);

	UFUNCTION()
	void HandleRelayStateChanged(int32 RelayNumber, bool bOn);

	UFUNCTION()
	void HandleInputStateChanged(int32 Channel, bool bOn);

	UFUNCTION()
	void HandleConveyorStateChanged(EMLConveyorState NewState);

	UFUNCTION()
	void HandleQueueChanged();

	UFUNCTION()
	void HandlePulse();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleStopClicked();
};
