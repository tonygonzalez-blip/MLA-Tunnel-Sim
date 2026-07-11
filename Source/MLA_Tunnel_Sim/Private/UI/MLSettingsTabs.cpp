// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLSettingsTabs.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "MicrologicControllerSubsystem.h"
#include "Misc/Paths.h"
#include "UI/MLUiText.h"

namespace
{
	// Micrologic blue (#2B5DD7) for the selected backup row.
	const FLinearColor SelectedRowColor(0.169f, 0.365f, 0.843f);
	const FLinearColor UnselectedRowColor(1.f, 1.f, 1.f);

	int32 SpinToInt(const USpinBox* SpinBox, int32 Fallback)
	{
		return SpinBox ? FMath::Max(0, FMath::RoundToInt32(SpinBox->GetValue())) : Fallback;
	}

	float SpinToFloat(const USpinBox* SpinBox, float Fallback)
	{
		return SpinBox ? FMath::Max(0.f, SpinBox->GetValue()) : Fallback;
	}
}

// ---------------------------------------------------------------------------
// UMLSettingsTabBase
// ---------------------------------------------------------------------------

void UMLSettingsTabBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Save)
	{
		Button_Save->OnClicked.AddUniqueDynamic(this, &UMLSettingsTabBase::HandleSaveClicked);
	}

	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.AddUniqueDynamic(this, &UMLSettingsTabBase::HandleConfigChanged);
	}
}

void UMLSettingsTabBase::NativeDestruct()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->OnConfigChanged.RemoveDynamic(this, &UMLSettingsTabBase::HandleConfigChanged);
	}

	Super::NativeDestruct();
}

void UMLSettingsTabBase::HandleSaveClicked()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	StageFromUi();
	// Per-tab Save matches the real UI: stage this tab's section and commit.
	Controller->Commit();
}

void UMLSettingsTabBase::HandleConfigChanged()
{
	RefreshFromController();
}

// ---------------------------------------------------------------------------
// Communications
// ---------------------------------------------------------------------------

void UMLSettingsCommunicationsTab::RefreshFromController()
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FMLCommunicationsSettings& Settings = Controller->GetStagedConfig().Communications;

	if (SpinBox_NumInputBoards)
	{
		SpinBox_NumInputBoards->SetValue(static_cast<float>(Settings.NumInputBoards));
	}
	if (SpinBox_NumRelayBoards)
	{
		SpinBox_NumRelayBoards->SetValue(static_cast<float>(Settings.NumRelayBoards));
	}
	if (EditableTextBox_InputBoardPort)
	{
		EditableTextBox_InputBoardPort->SetText(FText::FromString(Settings.InputBoardPort));
	}
	if (EditableTextBox_OutputBoardPort)
	{
		EditableTextBox_OutputBoardPort->SetText(FText::FromString(Settings.OutputBoardPort));
	}
	if (EditableTextBox_KeypadPort)
	{
		EditableTextBox_KeypadPort->SetText(FText::FromString(Settings.KeypadPort));
	}
	if (CheckBox_ExitDoor)
	{
		CheckBox_ExitDoor->SetIsChecked(Settings.bExitDoorEnabled);
	}
}

void UMLSettingsCommunicationsTab::StageFromUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Start from the staged copy so fields this tab does not own —
	// notably SonarAddress (edited on the Sonar tab) — are preserved.
	FMLCommunicationsSettings Settings = Controller->GetStagedConfig().Communications;

	Settings.NumInputBoards = FMath::Max(1, SpinToInt(SpinBox_NumInputBoards, Settings.NumInputBoards));
	Settings.NumRelayBoards = FMath::Max(1, SpinToInt(SpinBox_NumRelayBoards, Settings.NumRelayBoards));
	if (EditableTextBox_InputBoardPort)
	{
		Settings.InputBoardPort = EditableTextBox_InputBoardPort->GetText().ToString();
	}
	if (EditableTextBox_OutputBoardPort)
	{
		Settings.OutputBoardPort = EditableTextBox_OutputBoardPort->GetText().ToString();
	}
	if (EditableTextBox_KeypadPort)
	{
		Settings.KeypadPort = EditableTextBox_KeypadPort->GetText().ToString();
	}
	if (CheckBox_ExitDoor)
	{
		Settings.bExitDoorEnabled = CheckBox_ExitDoor->IsChecked();
	}

	Controller->SetStagedCommunications(Settings);
}

// ---------------------------------------------------------------------------
// Conveyor
// ---------------------------------------------------------------------------

void UMLSettingsConveyorTab::RefreshFromController()
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FMLConveyorSettings& Settings = Controller->GetStagedConfig().Conveyor;

	if (SpinBox_IPS)
	{
		SpinBox_IPS->SetValue(Settings.InchesPerSecond);
	}
	if (SpinBox_IPP)
	{
		SpinBox_IPP->SetValue(Settings.InchesPerPulse);
	}
	if (SpinBox_OnActivationService)
	{
		SpinBox_OnActivationService->SetValue(static_cast<float>(Settings.OnActivationService));
	}
	if (SpinBox_ShutOffService)
	{
		SpinBox_ShutOffService->SetValue(static_cast<float>(Settings.ShutOffService));
	}
	if (SpinBox_InactivityTimeout)
	{
		SpinBox_InactivityTimeout->SetValue(Settings.InactivityTimeoutSeconds);
	}
	if (SpinBox_HornTime)
	{
		SpinBox_HornTime->SetValue(Settings.HornTimeSeconds);
	}
	if (SpinBox_HornDelay)
	{
		SpinBox_HornDelay->SetValue(Settings.HornDelaySeconds);
	}
}

void UMLSettingsConveyorTab::StageFromUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	FMLConveyorSettings Settings = Controller->GetStagedConfig().Conveyor;

	Settings.InchesPerSecond = SpinToFloat(SpinBox_IPS, Settings.InchesPerSecond);
	Settings.InchesPerPulse = SpinToFloat(SpinBox_IPP, Settings.InchesPerPulse);
	Settings.OnActivationService = SpinToInt(SpinBox_OnActivationService, Settings.OnActivationService);
	Settings.ShutOffService = SpinToInt(SpinBox_ShutOffService, Settings.ShutOffService);
	Settings.InactivityTimeoutSeconds = SpinToFloat(SpinBox_InactivityTimeout, Settings.InactivityTimeoutSeconds);
	Settings.HornTimeSeconds = SpinToFloat(SpinBox_HornTime, Settings.HornTimeSeconds);
	Settings.HornDelaySeconds = SpinToFloat(SpinBox_HornDelay, Settings.HornDelaySeconds);

	Controller->SetStagedConveyor(Settings);
}

// ---------------------------------------------------------------------------
// Anti-Collision
// ---------------------------------------------------------------------------

void UMLSettingsAntiCollisionTab::RefreshFromController()
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FMLAntiCollisionSettings& Settings = Controller->GetStagedConfig().AntiCollision;

	if (SpinBox_Relay)
	{
		SpinBox_Relay->SetValue(static_cast<float>(Settings.RelayNumber));
	}
	if (SpinBox_AfterClearsService)
	{
		SpinBox_AfterClearsService->SetValue(static_cast<float>(Settings.AfterClearsActivateService));
	}
	if (SpinBox_SlowDownService)
	{
		SpinBox_SlowDownService->SetValue(static_cast<float>(Settings.SlowDownService));
	}
	if (SpinBox_SlowDownTime)
	{
		SpinBox_SlowDownTime->SetValue(Settings.SlowDownTimeSeconds);
	}
	if (SpinBox_SlowDownHornService)
	{
		SpinBox_SlowDownHornService->SetValue(static_cast<float>(Settings.SlowDownHornService));
	}
}

void UMLSettingsAntiCollisionTab::StageFromUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	FMLAntiCollisionSettings Settings = Controller->GetStagedConfig().AntiCollision;

	Settings.RelayNumber = SpinToInt(SpinBox_Relay, Settings.RelayNumber);
	Settings.AfterClearsActivateService = SpinToInt(SpinBox_AfterClearsService, Settings.AfterClearsActivateService);
	Settings.SlowDownService = SpinToInt(SpinBox_SlowDownService, Settings.SlowDownService);
	Settings.SlowDownTimeSeconds = SpinToFloat(SpinBox_SlowDownTime, Settings.SlowDownTimeSeconds);
	Settings.SlowDownHornService = SpinToInt(SpinBox_SlowDownHornService, Settings.SlowDownHornService);

	Controller->SetStagedAntiCollision(Settings);
}

// ---------------------------------------------------------------------------
// Roller/Defaults
// ---------------------------------------------------------------------------

void UMLSettingsRollerDefaultsTab::NativeConstruct()
{
	// Populate the combos BEFORE the base construct runs RefreshFromController
	// via Super (base order: Super::NativeConstruct → RefreshFromController).
	MLUi::PopulateEnumCombo(ComboBoxString_RollerMode, MLUi::RollerModeOptions(), 0);
	MLUi::PopulateEnumCombo(ComboBoxString_QueueMode, MLUi::QueueModeOptions(), 0);

	Super::NativeConstruct();
}

void UMLSettingsRollerDefaultsTab::RefreshFromController()
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FMLRollerDefaults& Settings = Controller->GetStagedConfig().RollerDefaults;

	if (SpinBox_MinCarLength)
	{
		SpinBox_MinCarLength->SetValue(Settings.MinCarLengthFeet);
	}
	if (SpinBox_MaxCarLength)
	{
		SpinBox_MaxCarLength->SetValue(Settings.MaxCarLengthFeet);
	}
	if (SpinBox_AverageCarLength)
	{
		SpinBox_AverageCarLength->SetValue(Settings.AverageCarLengthFeet);
	}
	if (ComboBoxString_RollerMode && ComboBoxString_RollerMode->GetOptionCount() > 0)
	{
		ComboBoxString_RollerMode->SetSelectedIndex(static_cast<int32>(Settings.RollerMode));
	}
	if (SpinBox_Up)
	{
		SpinBox_Up->SetValue(Settings.UpFeet);
	}
	if (SpinBox_Down)
	{
		SpinBox_Down->SetValue(Settings.DownFeet);
	}
	if (SpinBox_UpAgain)
	{
		SpinBox_UpAgain->SetValue(Settings.UpAgainFeet);
	}
	if (CheckBox_NeedsCarQueued)
	{
		CheckBox_NeedsCarQueued->SetIsChecked(Settings.bNeedsCarQueuedForRollerRequest);
	}
	if (SpinBox_DefaultDebounce)
	{
		SpinBox_DefaultDebounce->SetValue(Settings.DefaultInputDebounceSeconds);
	}
	if (ComboBoxString_QueueMode && ComboBoxString_QueueMode->GetOptionCount() > 0)
	{
		ComboBoxString_QueueMode->SetSelectedIndex(static_cast<int32>(Settings.QueueMode));
	}
	if (SpinBox_ServiceOnQueued)
	{
		SpinBox_ServiceOnQueued->SetValue(static_cast<float>(Settings.ServiceOnQueued));
	}
	if (SpinBox_DefaultWash)
	{
		SpinBox_DefaultWash->SetValue(static_cast<float>(Settings.DefaultWashService));
	}
}

void UMLSettingsRollerDefaultsTab::StageFromUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	FMLRollerDefaults Settings = Controller->GetStagedConfig().RollerDefaults;

	Settings.MinCarLengthFeet = SpinToFloat(SpinBox_MinCarLength, Settings.MinCarLengthFeet);
	Settings.MaxCarLengthFeet = SpinToFloat(SpinBox_MaxCarLength, Settings.MaxCarLengthFeet);
	Settings.AverageCarLengthFeet = SpinToFloat(SpinBox_AverageCarLength, Settings.AverageCarLengthFeet);
	if (ComboBoxString_RollerMode)
	{
		Settings.RollerMode = static_cast<EMLRollerMode>(MLUi::GetComboIndex(ComboBoxString_RollerMode, MLUi::RollerModeOptions().Num()));
	}
	Settings.UpFeet = SpinToFloat(SpinBox_Up, Settings.UpFeet);
	Settings.DownFeet = SpinToFloat(SpinBox_Down, Settings.DownFeet);
	Settings.UpAgainFeet = SpinToFloat(SpinBox_UpAgain, Settings.UpAgainFeet);
	if (CheckBox_NeedsCarQueued)
	{
		Settings.bNeedsCarQueuedForRollerRequest = CheckBox_NeedsCarQueued->IsChecked();
	}
	Settings.DefaultInputDebounceSeconds = SpinToFloat(SpinBox_DefaultDebounce, Settings.DefaultInputDebounceSeconds);
	if (ComboBoxString_QueueMode)
	{
		Settings.QueueMode = static_cast<EMLQueueMode>(MLUi::GetComboIndex(ComboBoxString_QueueMode, MLUi::QueueModeOptions().Num()));
	}
	Settings.ServiceOnQueued = SpinToInt(SpinBox_ServiceOnQueued, Settings.ServiceOnQueued);
	Settings.DefaultWashService = SpinToInt(SpinBox_DefaultWash, Settings.DefaultWashService);

	Controller->SetStagedRollerDefaults(Settings);
}

// ---------------------------------------------------------------------------
// Security
// ---------------------------------------------------------------------------

void UMLSettingsSecurityTab::RefreshFromController()
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FMLSecuritySettings& Settings = Controller->GetStagedConfig().Security;

	if (CheckBox_RequirePin)
	{
		CheckBox_RequirePin->SetIsChecked(Settings.bRequirePinForRelayOverride);
	}
	if (EditableTextBox_Pin)
	{
		EditableTextBox_Pin->SetText(FText::FromString(Settings.PinCode));
	}
	if (EditableTextBox_UiPassword)
	{
		EditableTextBox_UiPassword->SetText(FText::FromString(Settings.UiPassword));
	}
}

void UMLSettingsSecurityTab::StageFromUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// Start from the staged copy so the web-login credentials (Username /
	// Password), which this tab does not edit, are preserved.
	FMLSecuritySettings Settings = Controller->GetStagedConfig().Security;

	if (CheckBox_RequirePin)
	{
		Settings.bRequirePinForRelayOverride = CheckBox_RequirePin->IsChecked();
	}
	if (EditableTextBox_Pin)
	{
		Settings.PinCode = EditableTextBox_Pin->GetText().ToString();
	}
	if (EditableTextBox_UiPassword)
	{
		Settings.UiPassword = EditableTextBox_UiPassword->GetText().ToString();
	}

	Controller->SetStagedSecurity(Settings);
}

// ---------------------------------------------------------------------------
// Backup/Restore
// ---------------------------------------------------------------------------

void UMLBackupRowButton::InitRow(const FString& InFilePath)
{
	FilePath = InFilePath;
	OnClicked.AddUniqueDynamic(this, &UMLBackupRowButton::HandleClicked);
}

void UMLBackupRowButton::HandleClicked()
{
	OnRowClicked.Broadcast(FilePath);
}

void UMLSettingsBackupRestoreTab::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Backup)
	{
		Button_Backup->OnClicked.AddUniqueDynamic(this, &UMLSettingsBackupRestoreTab::HandleBackupClicked);
	}
	if (Button_Restore)
	{
		Button_Restore->OnClicked.AddUniqueDynamic(this, &UMLSettingsBackupRestoreTab::HandleRestoreClicked);
	}
	if (Button_Reset)
	{
		Button_Reset->OnClicked.AddUniqueDynamic(this, &UMLSettingsBackupRestoreTab::HandleResetClicked);
	}
	if (Button_Commit)
	{
		Button_Commit->OnClicked.AddUniqueDynamic(this, &UMLSettingsBackupRestoreTab::HandleCommitClicked);
	}
	if (Button_CommitReload)
	{
		Button_CommitReload->OnClicked.AddUniqueDynamic(this, &UMLSettingsBackupRestoreTab::HandleCommitReloadClicked);
	}
}

void UMLSettingsBackupRestoreTab::RefreshFromController()
{
	RebuildBackupList();
}

void UMLSettingsBackupRestoreTab::RebuildBackupList()
{
	if (!ScrollBox_Backups || !WidgetTree)
	{
		return;
	}

	ScrollBox_Backups->ClearChildren();
	BackupRows.Empty();

	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	for (const FString& FilePath : Controller->ListBackupFiles())
	{
		UMLBackupRowButton* RowButton = WidgetTree->ConstructWidget<UMLBackupRowButton>(UMLBackupRowButton::StaticClass());
		if (!RowButton)
		{
			continue;
		}

		RowButton->InitRow(FilePath);
		RowButton->OnRowClicked.AddUniqueDynamic(this, &UMLSettingsBackupRestoreTab::HandleBackupRowClicked);

		if (UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Label->SetText(FText::FromString(FPaths::GetCleanFilename(FilePath)));
			RowButton->SetContent(Label);
		}

		ScrollBox_Backups->AddChild(RowButton);
		BackupRows.Add(RowButton);
	}

	// Drop a stale selection if its file disappeared.
	if (!SelectedBackupPath.IsEmpty())
	{
		const bool bStillExists = BackupRows.ContainsByPredicate([this](const UMLBackupRowButton* Row)
		{
			return Row && Row->GetFilePath() == SelectedBackupPath;
		});
		if (!bStillExists)
		{
			SelectedBackupPath.Reset();
		}
	}

	UpdateSelectionHighlight();
}

void UMLSettingsBackupRestoreTab::UpdateSelectionHighlight()
{
	for (UMLBackupRowButton* Row : BackupRows)
	{
		if (Row)
		{
			Row->SetBackgroundColor(Row->GetFilePath() == SelectedBackupPath ? SelectedRowColor : UnselectedRowColor);
		}
	}
}

void UMLSettingsBackupRestoreTab::SetStatus(const FString& Message)
{
	if (TextBlock_Status)
	{
		TextBlock_Status->SetText(FText::FromString(Message));
	}
}

void UMLSettingsBackupRestoreTab::HandleBackupClicked()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	const FString BackupName = EditableTextBox_BackupName ? EditableTextBox_BackupName->GetText().ToString() : FString();
	const FString WrittenPath = Controller->BackupToXmlFile(BackupName);

	if (WrittenPath.IsEmpty())
	{
		SetStatus(TEXT("Backup failed."));
	}
	else
	{
		SetStatus(FString::Printf(TEXT("Backup written to %s"), *WrittenPath));
	}

	RebuildBackupList();
}

void UMLSettingsBackupRestoreTab::HandleBackupRowClicked(const FString& FilePath)
{
	SelectedBackupPath = FilePath;
	UpdateSelectionHighlight();
}

void UMLSettingsBackupRestoreTab::HandleRestoreClicked()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	if (SelectedBackupPath.IsEmpty())
	{
		SetStatus(TEXT("Select a backup file first."));
		return;
	}

	if (Controller->RestoreFromXmlFile(SelectedBackupPath))
	{
		SetStatus(TEXT("Restored — press Commit + Reload to apply."));
	}
	else
	{
		SetStatus(TEXT("Restore failed — could not read backup file."));
	}
}

void UMLSettingsBackupRestoreTab::HandleResetClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->ResetConfiguration();
		SetStatus(TEXT("Configuration reset to factory defaults."));
	}
}

void UMLSettingsBackupRestoreTab::HandleCommitClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->Commit();
		SetStatus(TEXT("Committed."));
	}
}

void UMLSettingsBackupRestoreTab::HandleCommitReloadClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->CommitAndReload();
		SetStatus(TEXT("Committed and reloaded."));
	}
}

// ---------------------------------------------------------------------------
// Sonar
// ---------------------------------------------------------------------------

void UMLSettingsSonarTab::NativeConstruct()
{
	Super::NativeConstruct();

	if (TextBlock_Note)
	{
		TextBlock_Note->SetText(FText::FromString(
			TEXT("If this location does not utilize Micrologic sonar, this section can be ignored.")));
	}
}

void UMLSettingsSonarTab::RefreshFromController()
{
	const UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	if (EditableTextBox_SonarAddress)
	{
		EditableTextBox_SonarAddress->SetText(FText::FromString(Controller->GetStagedConfig().Communications.SonarAddress));
	}
}

void UMLSettingsSonarTab::StageFromUi()
{
	UMicrologicControllerSubsystem* Controller = GetController();
	if (!Controller)
	{
		return;
	}

	// SonarAddress lives on the Communications settings struct; preserve the
	// rest of that section.
	FMLCommunicationsSettings Settings = Controller->GetStagedConfig().Communications;

	if (EditableTextBox_SonarAddress)
	{
		Settings.SonarAddress = EditableTextBox_SonarAddress->GetText().ToString();
	}

	Controller->SetStagedCommunications(Settings);
}
