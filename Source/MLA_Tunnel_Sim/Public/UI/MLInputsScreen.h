// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "Templates/SubclassOf.h"
#include "MLInputsScreen.generated.h"

class UButton;
class UMLInputRow;
class UScrollBox;

/**
 * Inputs page: one UMLInputRow per staged input; Save commits the staged
 * config to the live controller.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLInputsScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

public:
	/** Row widget blueprint (parent class UMLInputRow) the designer assigns. */
	UPROPERTY(EditAnywhere, Category = "Micrologic|UI")
	TSubclassOf<UMLInputRow> InputRowClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void RefreshFromController() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Inputs;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Save;

private:
	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleConfigChanged();
};
