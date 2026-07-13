// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "MLSettingsScreen.generated.h"

class UButton;
class UWidgetSwitcher;

/**
 * Settings page: tab strip over WidgetSwitcher_Tabs.
 * Tab indices: 0 = Communications, 1 = Conveyor, 2 = Anti-Collision,
 * 3 = Roller/Defaults, 4 = Security, 5 = Backup/Restore, 6 = Sonar.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsScreen : public UMLControllerScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Tabs;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabCommunications;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabConveyor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabAntiCollision;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabRollerDefaults;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabSecurity;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabBackupRestore;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabSonar;

private:
	void SetTab(int32 Index);

	UFUNCTION()
	void HandleTabCommunications();

	UFUNCTION()
	void HandleTabConveyor();

	UFUNCTION()
	void HandleTabAntiCollision();

	UFUNCTION()
	void HandleTabRollerDefaults();

	UFUNCTION()
	void HandleTabSecurity();

	UFUNCTION()
	void HandleTabBackupRestore();

	UFUNCTION()
	void HandleTabSonar();
};
