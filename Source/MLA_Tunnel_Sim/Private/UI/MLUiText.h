// Copyright Micrologic Associates. All Rights Reserved.
//
// Shared display strings, colors, and small helpers for the Micrologic
// controller UI widgets. Non-UObject; plain functions returning shared
// arrays so every ComboBoxString uses identical option text, in enum order
// (selected index == static_cast<int32>(enum value)).

#pragma once

#include "CoreMinimal.h"
#include "MicrologicTypes.h"

class UComboBoxString;

namespace MLUi
{
	// ---- Option lists (index == enum value) -------------------------------

	/** EMLInputType display strings, matching the real V2 web UI. */
	const TArray<FString>& InputTypeOptions();

	/** EMLRelayType display strings. */
	const TArray<FString>& RelayTypeOptions();

	/** EMLFunctionType display strings. */
	const TArray<FString>& FunctionTypeOptions();

	/** EMLModifierType display strings. */
	const TArray<FString>& ModifierTypeOptions();

	/** EMLServiceType display strings. */
	const TArray<FString>& ServiceTypeOptions();

	/** EMLQueueMode display strings. */
	const TArray<FString>& QueueModeOptions();

	/** EMLRollerMode display strings. */
	const TArray<FString>& RollerModeOptions();

	/** EMLTurnReference display strings. */
	const TArray<FString>& TurnReferenceOptions();

	/** EMLCommand display strings. */
	const TArray<FString>& CommandOptions();

	// ---- State text --------------------------------------------------------

	FString ConveyorStateText(EMLConveyorState State);
	FString StopReasonText(EMLStopReason Reason);

	// ---- Colors (dashboard / boards LED conventions) -----------------------

	/** LED / relay ON. */
	inline const FLinearColor OnColor(0.1f, 0.8f, 0.2f);

	/** LED / relay OFF (dim). */
	inline const FLinearColor OffColor(0.15f, 0.15f, 0.15f);

	/** Fault / stop red. */
	inline const FLinearColor FaultColor(0.9f, 0.15f, 0.1f);

	// ---- Helpers ------------------------------------------------------------

	/**
	 * Clears a combo box and repopulates it with Options, then selects
	 * SelectedIndex (clamped; -1 leaves nothing selected). Safe on nullptr.
	 */
	void PopulateEnumCombo(UComboBoxString* Combo, const TArray<FString>& Options, int32 SelectedIndex = 0);

	/** Combo selected index clamped into a uint8 enum range. Safe on nullptr (returns 0). */
	int32 GetComboIndex(const UComboBoxString* Combo, int32 NumOptions);

	/** "1,9,13" from {1, 9, 13}. */
	FString IntArrayToCsv(const TArray<int32>& Values);

	/** {1, 9, 13} from "1, 9,13,junk" — non-numeric entries ignored. */
	TArray<int32> CsvToIntArray(const FString& Csv);
}
