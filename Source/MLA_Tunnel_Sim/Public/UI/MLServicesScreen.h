// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "Templates/SubclassOf.h"
#include "MLServicesScreen.generated.h"

class UButton;
class UMLServiceEditPanel;
class UMLServiceRow;
class UScrollBox;

/**
 * Services page: one UMLServiceRow per staged service, plus the shared edit
 * panel (a UMLServiceEditPanel subwidget named Widget_EditPanel).
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLServicesScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

public:
	/** Row widget blueprint (parent class UMLServiceRow). */
	UPROPERTY(EditAnywhere, Category = "Micrologic|UI")
	TSubclassOf<UMLServiceRow> ServiceRowClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void RefreshFromController() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Services;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMLServiceEditPanel> Widget_EditPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_AddService;

private:
	UFUNCTION()
	void HandleEditRequested(int32 ServiceNumber);

	UFUNCTION()
	void HandleAddServiceClicked();

	UFUNCTION()
	void HandlePanelSaved();

	UFUNCTION()
	void HandleConfigChanged();
};
