// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MicrologicTypes.h"
#include "Types/SlateEnums.h"
#include "MLModifierRow.generated.h"

class UButton;
class UComboBoxString;
class USpinBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMLModifierRowChanged, int32, ModifierIndex, FMLModifierConfig, Modifier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLModifierRowRemove, int32, ModifierIndex);

/**
 * One modifier row inside the relay edit panel. Edits are reported back to
 * the panel's working copy by index; the row never touches the subsystem.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLModifierRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load the row: which index of the panel's working array it edits + values. */
	void SetModifier(int32 InIndex, const FMLModifierConfig& InModifier);

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLModifierRowChanged OnModifierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|UI")
	FMLModifierRowRemove OnRemoveRequested;

protected:
	virtual void NativeConstruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_Type;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Start;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Length;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Remove;

private:
	int32 ModifierIndex = INDEX_NONE;
	FMLModifierConfig Modifier;

	void NotifyChanged();

	UFUNCTION()
	void HandleTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleStartCommitted(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleLengthCommitted(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleRemoveClicked();
};
