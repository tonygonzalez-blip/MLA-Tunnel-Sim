// Copyright Micrologic Associates. All Rights Reserved.
//
// The seven Settings tabs. Each tab loads current STAGED values in
// RefreshFromController (and again whenever the controller broadcasts
// OnConfigChanged), and its Save button stages its own section
// (SetStagedXxx) then commits — matching the real web UI's per-tab
// "Save" button at the bottom right.

#pragma once

#include "CoreMinimal.h"
#include "UI/MLControllerScreenBase.h"
#include "Components/Button.h"
#include "MLSettingsTabs.generated.h"

class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UScrollBox;
class USpinBox;
class UTextBlock;

// ---------------------------------------------------------------------------
// Shared save/refresh plumbing
// ---------------------------------------------------------------------------

/**
 * Base for the settings tabs: wires Button_Save (when bound by the derived
 * class) and re-runs RefreshFromController on OnConfigChanged.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsTabBase : public UMLControllerScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Derived tabs stage their section here; base then calls Commit(). */
	virtual void StageFromUi() {}

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Save;

private:
	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleConfigChanged();
};

// ---------------------------------------------------------------------------
// Communications
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsCommunicationsTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void RefreshFromController() override;
	virtual void StageFromUi() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_NumInputBoards;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_NumRelayBoards;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_InputBoardPort;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_OutputBoardPort;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_KeypadPort;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_ExitDoor;
};

// ---------------------------------------------------------------------------
// Conveyor
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsConveyorTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void RefreshFromController() override;
	virtual void StageFromUi() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_IPS;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_IPP;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_OnActivationService;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_ShutOffService;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_InactivityTimeout;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_HornTime;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_HornDelay;
};

// ---------------------------------------------------------------------------
// Anti-Collision
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsAntiCollisionTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void RefreshFromController() override;
	virtual void StageFromUi() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Relay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_AfterClearsService;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_SlowDownService;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_SlowDownTime;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_SlowDownHornService;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_ConveyorStall;
};

// ---------------------------------------------------------------------------
// Roller/Defaults
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsRollerDefaultsTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void RefreshFromController() override;
	virtual void StageFromUi() override;

	// Micrologic presets (min/max/average car length) — displayed and still
	// editable, matching the real UI, but techs are told not to change them.

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_MinCarLength;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_MaxCarLength;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_AverageCarLength;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_RollerMode;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Up;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_Down;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_UpAgain;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_NeedsCarQueued;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_DefaultDebounce;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBoxString_QueueMode;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_ServiceOnQueued;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpinBox_DefaultWash;
};

// ---------------------------------------------------------------------------
// Security
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsSecurityTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void RefreshFromController() override;
	virtual void StageFromUi() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_RequirePin;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Pin;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_UiPassword;
};

// ---------------------------------------------------------------------------
// Backup/Restore
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLBackupRowClicked, const FString&, FilePath);

/**
 * List-row button for the backups list. Dynamic delegates carry no sender, so
 * the button remembers its own file path and reports clicks with it.
 */
UCLASS()
class MLA_TUNNEL_SIM_API UMLBackupRowButton : public UButton
{
	GENERATED_BODY()

public:
	void InitRow(const FString& InFilePath);

	UPROPERTY()
	FMLBackupRowClicked OnRowClicked;

	FString GetFilePath() const { return FilePath; }

private:
	FString FilePath;

	UFUNCTION()
	void HandleClicked();
};

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsBackupRestoreTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void RefreshFromController() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_BackupName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Backup;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Backups;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Restore;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Reset;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Commit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_CommitReload;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Status;

private:
	FString SelectedBackupPath;

	/** Row buttons currently in ScrollBox_Backups (for selection highlight). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLBackupRowButton>> BackupRows;

	void RebuildBackupList();
	void UpdateSelectionHighlight();
	void SetStatus(const FString& Message);

	UFUNCTION()
	void HandleBackupClicked();

	UFUNCTION()
	void HandleBackupRowClicked(const FString& FilePath);

	UFUNCTION()
	void HandleRestoreClicked();

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleCommitClicked();

	UFUNCTION()
	void HandleCommitReloadClicked();
};

// ---------------------------------------------------------------------------
// Sonar
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLSettingsSonarTab : public UMLSettingsTabBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void RefreshFromController() override;
	virtual void StageFromUi() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_SonarAddress;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Note;
};
