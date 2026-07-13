// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "MLServiceRow.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLServiceEditRequested, int32, ServiceNumber);

/**
 * One row on the Services page. Send fires the service on the live
 * controller so trainees can trigger services straight from the UI.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLServiceRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load this row from a staged service config. */
	void SetService(const FMLServiceConfig& InConfig);

	/** Fired when the row's Edit button is clicked. */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLServiceEditRequested OnEditRequested;

protected:
	virtual void NativeConstruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Number;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Type;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Edit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Send;

private:
	FMLServiceConfig Config;

	class UMicrologicControllerSubsystem* GetController() const;

	UFUNCTION()
	void HandleEditClicked();

	UFUNCTION()
	void HandleSendClicked();
};
