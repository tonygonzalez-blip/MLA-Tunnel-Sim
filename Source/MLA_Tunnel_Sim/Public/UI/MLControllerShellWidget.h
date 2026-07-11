// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "MLControllerShellWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UMLLoginScreen;
class UTextBlock;
class UWidgetSwitcher;

/**
 * The browser-chrome frame around the whole controller web UI.
 *
 * WidgetSwitcher_Root: 0 = login page, 1 = main app.
 * WidgetSwitcher_Screens: 0 = Dashboard, 1 = Settings, 2 = Inputs,
 * 3 = Relays, 4 = Services, 5 = Hardware Boards.
 *
 * Also hosts the VNC-style lock overlay: LockUi() (or 300 s of idle while
 * logged in) shows Border_LockOverlay; Button_Unlock validates against
 * UMicrologicControllerSubsystem::ValidateUiPassword.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLControllerShellWidget : public UMLControllerScreenBase
{
	GENERATED_BODY()

public:
	/** Show the lock overlay (the physical 'L'-key lock on the real box). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|UI")
	void LockUi();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- Designer bindings (all optional; null-checked before use) --------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_AddressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMLLoginScreen> Widget_Login;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Screens;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_NavDashboard;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_NavSettings;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_NavInputs;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_NavRelays;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_NavServices;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_NavBoards;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_LockOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_UnlockPassword;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Unlock;

private:
	/** Seconds of idle (no nav interaction) before the UI auto-locks. */
	static constexpr float AutoLockSeconds = 300.f;

	bool bLoggedIn = false;
	bool bLocked = false;
	float IdleSeconds = 0.f;

	void SetScreen(int32 Index);

	UFUNCTION()
	void HandleLoginSucceeded();

	UFUNCTION()
	void HandleNavDashboard();

	UFUNCTION()
	void HandleNavSettings();

	UFUNCTION()
	void HandleNavInputs();

	UFUNCTION()
	void HandleNavRelays();

	UFUNCTION()
	void HandleNavServices();

	UFUNCTION()
	void HandleNavBoards();

	UFUNCTION()
	void HandleUnlockClicked();
};
