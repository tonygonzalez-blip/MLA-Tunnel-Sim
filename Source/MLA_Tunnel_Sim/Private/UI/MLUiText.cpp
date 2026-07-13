// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLUiText.h"

#include "Components/ComboBoxString.h"

namespace MLUi
{

const TArray<FString>& InputTypeOptions()
{
	static const TArray<FString> Options = {
		TEXT("None"),
		TEXT("Trigger"),
		TEXT("Conveyor"),
		TEXT("Roller Position"),
		TEXT("Tire Switch"),
		TEXT("Upper Entry"),
		TEXT("Anti Collision"),
		TEXT("Stall"),
		TEXT("Entry"),
		TEXT("Exit Door")
	};
	return Options;
}

const TArray<FString>& RelayTypeOptions()
{
	static const TArray<FString> Options = {
		TEXT("Normal"),
		TEXT("Roller"),
		TEXT("Horn"),
		TEXT("Conveyor")
	};
	return Options;
}

const TArray<FString>& FunctionTypeOptions()
{
	static const TArray<FString> Options = {
		TEXT("None"),
		TEXT("Vehicle Length"),
		TEXT("Front Of Vehicle"),
		TEXT("Rear Of Vehicle"),
		TEXT("Front Half Of Vehicle"),
		TEXT("Rear Half Of Vehicle"),
		TEXT("All Tires"),
		TEXT("Front Tires"),
		TEXT("Rear Tires"),
		TEXT("Pickup Bed"),
		TEXT("Light")
	};
	return Options;
}

const TArray<FString>& ModifierTypeOptions()
{
	static const TArray<FString> Options = {
		TEXT("Front & Rear Only"),
		TEXT("Bump"),
		TEXT("Mirror Bump"),
		TEXT("Open Pickup Bed"),
		TEXT("Rear Of Car"),
		TEXT("Front Of Car")
	};
	return Options;
}

const TArray<FString>& ServiceTypeOptions()
{
	static const TArray<FString> Options = {
		TEXT("Wash"),
		TEXT("Service"),
		TEXT("Macro"),
		TEXT("De-programmable"),
		TEXT("Turn OFF"),
		TEXT("Turn ON"),
		TEXT("Momentarily On"),
		TEXT("Momentarily Off"),
		TEXT("Override"),
		TEXT("Toggler"),
		TEXT("Command")
	};
	return Options;
}

const TArray<FString>& QueueModeOptions()
{
	static const TArray<FString> Options = {
		TEXT("None"),
		TEXT("Random"),
		TEXT("Sequential")
	};
	return Options;
}

const TArray<FString>& RollerModeOptions()
{
	static const TArray<FString> Options = {
		TEXT("Manual/Front"),
		TEXT("Automatic/Rear")
	};
	return Options;
}

const TArray<FString>& TurnReferenceOptions()
{
	static const TArray<FString> Options = {
		TEXT("Before Front Of Vehicle"),
		TEXT("After Front Of Vehicle"),
		TEXT("Before Rear Of Vehicle"),
		TEXT("After Rear Of Vehicle")
	};
	return Options;
}

const TArray<FString>& CommandOptions()
{
	static const TArray<FString> Options = {
		TEXT("None"),
		TEXT("Accept Order"),
		TEXT("Cancel Order"),
		TEXT("Remove Last"),
		TEXT("Remove All"),
		TEXT("Roller Abort"),
		TEXT("Roller Request")
	};
	return Options;
}

FString ConveyorStateText(EMLConveyorState State)
{
	switch (State)
	{
	case EMLConveyorState::Stopped:     return TEXT("Stopped");
	case EMLConveyorState::HornDelay:   return TEXT("Horn Delay");
	case EMLConveyorState::Horn:        return TEXT("Horn");
	case EMLConveyorState::Running:     return TEXT("Running");
	case EMLConveyorState::SlowingDown: return TEXT("Slowing Down");
	default:                            return TEXT("Unknown");
	}
}

FString StopReasonText(EMLStopReason Reason)
{
	switch (Reason)
	{
	case EMLStopReason::None:          return TEXT("");
	case EMLStopReason::Manual:        return TEXT("Manual Stop");
	case EMLStopReason::StopCircuit:   return TEXT("Stop Circuit / E-Stop");
	case EMLStopReason::Stall:         return TEXT("Conveyor Stall");
	case EMLStopReason::ExitDoor:      return TEXT("Exit Door");
	case EMLStopReason::AntiCollision: return TEXT("Anti-Collision");
	case EMLStopReason::Inactivity:    return TEXT("Inactivity Timeout");
	default:                           return TEXT("");
	}
}

void PopulateEnumCombo(UComboBoxString* Combo, const TArray<FString>& Options, int32 SelectedIndex)
{
	if (!Combo)
	{
		return;
	}

	Combo->ClearOptions();
	for (const FString& Option : Options)
	{
		Combo->AddOption(Option);
	}

	if (Options.Num() > 0 && SelectedIndex >= 0)
	{
		Combo->SetSelectedIndex(FMath::Clamp(SelectedIndex, 0, Options.Num() - 1));
	}
}

int32 GetComboIndex(const UComboBoxString* Combo, int32 NumOptions)
{
	if (!Combo || NumOptions <= 0)
	{
		return 0;
	}
	return FMath::Clamp(Combo->GetSelectedIndex(), 0, NumOptions - 1);
}

FString IntArrayToCsv(const TArray<int32>& Values)
{
	FString Result;
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		if (Index > 0)
		{
			Result += TEXT(",");
		}
		Result += FString::FromInt(Values[Index]);
	}
	return Result;
}

TArray<int32> CsvToIntArray(const FString& Csv)
{
	TArray<int32> Result;

	TArray<FString> Parts;
	Csv.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/true);

	for (FString& Part : Parts)
	{
		Part.TrimStartAndEndInline();
		if (Part.IsEmpty())
		{
			continue;
		}

		int32 Value = 0;
		if (LexTryParseString(Value, *Part))
		{
			Result.Add(Value);
		}
	}
	return Result;
}

} // namespace MLUi
