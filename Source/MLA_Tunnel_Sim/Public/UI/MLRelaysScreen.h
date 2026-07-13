// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "Templates/SubclassOf.h"
#include "MLRelaysScreen.generated.h"

class UButton;
class UMLRelayEditPanel;
class UMLRelayRow;
class UScrollBox;

/**
 * Relays page: one UMLRelayRow per staged relay, plus the shared edit panel
 * (designer places a UMLRelayEditPanel subwidget named Widget_EditPanel,
 * hidden by default).
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLRelaysScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

public:
	/** Row widget blueprint (parent class UMLRelayRow). */
	UPROPERTY(EditAnywhere, Category = "Micrologic|UI")
	TSubclassOf<UMLRelayRow> RelayRowClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void RefreshFromController() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Relays;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMLRelayEditPanel> Widget_EditPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_AddRelay;

private:
	UFUNCTION()
	void HandleEditRequested(int32 RelayNumber);

	UFUNCTION()
	void HandleAddRelayClicked();

	UFUNCTION()
	void HandlePanelSaved();

	UFUNCTION()
	void HandleConfigChanged();
};
