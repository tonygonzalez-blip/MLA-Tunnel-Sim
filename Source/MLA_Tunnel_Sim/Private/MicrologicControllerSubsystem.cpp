// Copyright Micrologic Associates. All Rights Reserved.

#include "MicrologicControllerSubsystem.h"
#include "MicrologicSaveGame.h"
#include "MicrologicXml.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const TCHAR* UMicrologicControllerSubsystem::SaveSlotName = TEXT("MicrologicControllerConfig");

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMicrologicControllerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FMLControllerConfig Loaded;
	if (LoadConfigFromSlot(Loaded))
	{
		StagedConfig = Loaded;
	}
	else
	{
		StagedConfig = MakeFactoryDefaultConfig();
	}

	Core.SetConfig(StagedConfig, /*bReload=*/true);
	BindCoreEvents();
}

void UMicrologicControllerSubsystem::Deinitialize()
{
	SaveConfigToSlot();
	Super::Deinitialize();
}

void UMicrologicControllerSubsystem::BindCoreEvents()
{
	Core.OnRelayChanged.AddLambda([this](int32 RelayNumber, bool bOn)
	{
		OnRelayStateChanged.Broadcast(RelayNumber, bOn);
	});
	Core.OnInputChanged.AddLambda([this](int32 Channel, bool bOn)
	{
		OnInputStateChanged.Broadcast(Channel, bOn);
	});
	Core.OnConveyorStateChanged.AddLambda([this](EMLConveyorState NewState)
	{
		OnConveyorStateChanged.Broadcast(NewState);
	});
	Core.OnConveyorStopped.AddLambda([this](EMLStopReason Reason)
	{
		OnConveyorStopped.Broadcast(Reason);
	});
	Core.OnPulse.AddLambda([this]()
	{
		OnPulse.Broadcast();
	});
	Core.OnQueueChanged.AddLambda([this]()
	{
		OnQueueChanged.Broadcast();
	});
	Core.OnCarEntered.AddLambda([this](const FMLCarState& Car)
	{
		OnCarEntered.Broadcast(Car);
	});
	Core.OnCarMeasured.AddLambda([this](const FMLCarState& Car)
	{
		OnCarMeasured.Broadcast(Car);
	});
	Core.OnCarExited.AddLambda([this](const FMLCarState& Car)
	{
		OnCarExited.Broadcast(Car);
	});
	Core.OnServiceExecuted.AddLambda([this](int32 ServiceNumber, bool bAccepted)
	{
		OnServiceExecuted.Broadcast(ServiceNumber, bAccepted);
	});
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UMicrologicControllerSubsystem::Tick(float DeltaTime)
{
	Core.Advance(DeltaTime);
}

bool UMicrologicControllerSubsystem::IsTickable() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	return World != nullptr && World->IsGameWorld();
}

TStatId UMicrologicControllerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMicrologicControllerSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// Security
// ---------------------------------------------------------------------------

bool UMicrologicControllerSubsystem::ValidateLogin(const FString& Username, const FString& Password) const
{
	const FMLSecuritySettings& Security = Core.GetConfig().Security;
	return Username.Equals(Security.Username, ESearchCase::IgnoreCase) && Password == Security.Password;
}

bool UMicrologicControllerSubsystem::ValidateUiPassword(const FString& Password) const
{
	return Password == Core.GetConfig().Security.UiPassword;
}

bool UMicrologicControllerSubsystem::ValidatePin(const FString& Pin) const
{
	const FMLSecuritySettings& Security = Core.GetConfig().Security;
	return !Security.bRequirePinForRelayOverride || Pin == Security.PinCode;
}

bool UMicrologicControllerSubsystem::IsPinRequiredForRelayOverride() const
{
	return Core.GetConfig().Security.bRequirePinForRelayOverride;
}

// ---------------------------------------------------------------------------
// Staged configuration
// ---------------------------------------------------------------------------

void UMicrologicControllerSubsystem::SetStagedCommunications(const FMLCommunicationsSettings& Settings)
{
	StagedConfig.Communications = Settings;
}

void UMicrologicControllerSubsystem::SetStagedConveyor(const FMLConveyorSettings& Settings)
{
	StagedConfig.Conveyor = Settings;
}

void UMicrologicControllerSubsystem::SetStagedAntiCollision(const FMLAntiCollisionSettings& Settings)
{
	StagedConfig.AntiCollision = Settings;
}

void UMicrologicControllerSubsystem::SetStagedRollerDefaults(const FMLRollerDefaults& Settings)
{
	StagedConfig.RollerDefaults = Settings;
}

void UMicrologicControllerSubsystem::SetStagedSecurity(const FMLSecuritySettings& Settings)
{
	StagedConfig.Security = Settings;
}

void UMicrologicControllerSubsystem::SetStagedSim(const FMLSimSettings& Settings)
{
	StagedConfig.Sim = Settings;
}

void UMicrologicControllerSubsystem::UpsertStagedInput(const FMLInputConfig& Input)
{
	for (FMLInputConfig& Existing : StagedConfig.Inputs)
	{
		if (Existing.Channel == Input.Channel)
		{
			Existing = Input;
			return;
		}
	}
	StagedConfig.Inputs.Add(Input);
	StagedConfig.Inputs.Sort([](const FMLInputConfig& A, const FMLInputConfig& B)
	{
		return A.Channel < B.Channel;
	});
}

void UMicrologicControllerSubsystem::UpsertStagedRelay(const FMLRelayConfig& Relay)
{
	for (FMLRelayConfig& Existing : StagedConfig.Relays)
	{
		if (Existing.RelayNumber == Relay.RelayNumber)
		{
			Existing = Relay;
			return;
		}
	}
	StagedConfig.Relays.Add(Relay);
	StagedConfig.Relays.Sort([](const FMLRelayConfig& A, const FMLRelayConfig& B)
	{
		return A.RelayNumber < B.RelayNumber;
	});
}

void UMicrologicControllerSubsystem::UpsertStagedService(const FMLServiceConfig& Service)
{
	for (FMLServiceConfig& Existing : StagedConfig.Services)
	{
		if (Existing.ServiceNumber == Service.ServiceNumber)
		{
			Existing = Service;
			return;
		}
	}
	StagedConfig.Services.Add(Service);
	StagedConfig.Services.Sort([](const FMLServiceConfig& A, const FMLServiceConfig& B)
	{
		return A.ServiceNumber < B.ServiceNumber;
	});
}

void UMicrologicControllerSubsystem::RemoveStagedRelay(int32 RelayNumber)
{
	StagedConfig.Relays.RemoveAll([RelayNumber](const FMLRelayConfig& Relay)
	{
		return Relay.RelayNumber == RelayNumber;
	});
}

void UMicrologicControllerSubsystem::RemoveStagedService(int32 ServiceNumber)
{
	StagedConfig.Services.RemoveAll([ServiceNumber](const FMLServiceConfig& Service)
	{
		return Service.ServiceNumber == ServiceNumber;
	});
}

void UMicrologicControllerSubsystem::RemoveStagedInput(int32 Channel)
{
	StagedConfig.Inputs.RemoveAll([Channel](const FMLInputConfig& Input)
	{
		return Input.Channel == Channel;
	});
}

// ---------------------------------------------------------------------------
// Backup/Restore tab
// ---------------------------------------------------------------------------

void UMicrologicControllerSubsystem::Commit()
{
	Core.SetConfig(StagedConfig, /*bReload=*/false);
	SaveConfigToSlot();
	OnConfigChanged.Broadcast();
}

void UMicrologicControllerSubsystem::CommitAndReload()
{
	Core.SetConfig(StagedConfig, /*bReload=*/true);
	SaveConfigToSlot();
	OnConfigChanged.Broadcast();
}

void UMicrologicControllerSubsystem::ResetConfiguration()
{
	StagedConfig = MakeFactoryDefaultConfig();
	Core.SetConfig(StagedConfig, /*bReload=*/true);
	SaveConfigToSlot();
	OnConfigChanged.Broadcast();
}

void UMicrologicControllerSubsystem::RevertStaged()
{
	StagedConfig = Core.GetConfig();
	OnConfigChanged.Broadcast();
}

FString UMicrologicControllerSubsystem::BackupDirectory() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MicrologicBackups"));
}

FString UMicrologicControllerSubsystem::BackupToXmlFile(const FString& BackupName)
{
	FString SafeName = BackupName.IsEmpty() ? TEXT("Backup") : BackupName;
	// Strip characters that don't belong in file names.
	for (const TCHAR* Bad : { TEXT("/"), TEXT("\\"), TEXT(":"), TEXT("*"), TEXT("?"), TEXT("\""), TEXT("<"), TEXT(">"), TEXT("|") })
	{
		SafeName.ReplaceInline(Bad, TEXT("_"));
	}
	if (!SafeName.EndsWith(TEXT(".xml")))
	{
		SafeName += TEXT(".xml");
	}

	const FString Directory = BackupDirectory();
	IFileManager::Get().MakeDirectory(*Directory, /*Tree=*/true);
	const FString FullPath = FPaths::Combine(Directory, SafeName);

	const FString Xml = MicrologicXml::ConfigToXml(Core.GetConfig());
	if (!FFileHelper::SaveStringToFile(Xml, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return FString();
	}
	return FullPath;
}

bool UMicrologicControllerSubsystem::RestoreFromXmlFile(const FString& FilePath)
{
	FString Xml;
	if (!FFileHelper::LoadFileToString(Xml, *FilePath))
	{
		return false;
	}

	FMLControllerConfig Restored;
	if (!MicrologicXml::ConfigFromXml(Xml, Restored))
	{
		return false;
	}

	// Restore loads into the staged config; the user must Commit + Reload for
	// the backup to be written to the running controller.
	StagedConfig = Restored;
	OnConfigChanged.Broadcast();
	return true;
}

TArray<FString> UMicrologicControllerSubsystem::ListBackupFiles() const
{
	TArray<FString> Files;
	const FString Directory = BackupDirectory();
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Directory, TEXT("*.xml")), /*Files=*/true, /*Directories=*/false);

	for (FString& File : Files)
	{
		File = FPaths::Combine(Directory, File);
	}

	Files.Sort([](const FString& A, const FString& B)
	{
		return IFileManager::Get().GetTimeStamp(*A) > IFileManager::Get().GetTimeStamp(*B);
	});
	return Files;
}

FString UMicrologicControllerSubsystem::GetLiveConfigAsXml() const
{
	return MicrologicXml::ConfigToXml(Core.GetConfig());
}

// ---------------------------------------------------------------------------
// Save game
// ---------------------------------------------------------------------------

void UMicrologicControllerSubsystem::SaveConfigToSlot() const
{
	UMicrologicSaveGame* Save = Cast<UMicrologicSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UMicrologicSaveGame::StaticClass()));
	if (Save)
	{
		Save->Config = Core.GetConfig();
		UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
	}
}

bool UMicrologicControllerSubsystem::LoadConfigFromSlot(FMLControllerConfig& OutConfig) const
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		return false;
	}
	const UMicrologicSaveGame* Save = Cast<UMicrologicSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
	if (!Save)
	{
		return false;
	}
	OutConfig = Save->Config;
	return true;
}

// ---------------------------------------------------------------------------
// Hardware / controller pass-throughs
// ---------------------------------------------------------------------------

void UMicrologicControllerSubsystem::SetInputRaw(int32 Channel, bool bState)
{
	Core.SetInputRaw(Channel, bState);
}

void UMicrologicControllerSubsystem::SetInputRawByType(EMLInputType Type, bool bState)
{
	Core.SetInputRawByType(Type, bState);
}

void UMicrologicControllerSubsystem::FlagMeasuringCarOpenBed(bool bOpenBed)
{
	Core.FlagMeasuringCarOpenBed(bOpenBed);
}

void UMicrologicControllerSubsystem::PressStartCircuit(int32 Circuit)
{
	Core.PressStartCircuit(Circuit);
}

void UMicrologicControllerSubsystem::SetStopCircuit(int32 Circuit, bool bClosed)
{
	Core.SetStopCircuit(Circuit, bClosed);
}

bool UMicrologicControllerSubsystem::IsStopCircuitClosed(int32 Circuit) const
{
	return Core.IsStopCircuitClosed(Circuit);
}

bool UMicrologicControllerSubsystem::SetRelayOverride(int32 RelayNumber, EMLRelayOverride Override, const FString& Pin)
{
	if (!ValidatePin(Pin))
	{
		return false;
	}
	Core.SetRelayOverride(RelayNumber, Override);
	return true;
}

EMLRelayOverride UMicrologicControllerSubsystem::GetRelayOverride(int32 RelayNumber) const
{
	return Core.GetRelayOverride(RelayNumber);
}

int32 UMicrologicControllerSubsystem::SendOrder(const TArray<int32>& ServiceNumbers)
{
	return Core.SendOrder(ServiceNumbers);
}

bool UMicrologicControllerSubsystem::ExecuteService(int32 ServiceNumber)
{
	return Core.ExecuteService(ServiceNumber);
}

bool UMicrologicControllerSubsystem::ExecuteCommand(EMLCommand Command)
{
	return Core.ExecuteCommand(Command);
}

bool UMicrologicControllerSubsystem::RequestRoller()
{
	return Core.RequestRoller();
}

void UMicrologicControllerSubsystem::AbortRoller()
{
	Core.AbortRoller();
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

EMLConveyorState UMicrologicControllerSubsystem::GetConveyorState() const
{
	return Core.GetConveyorState();
}

EMLStopReason UMicrologicControllerSubsystem::GetLastStopReason() const
{
	return Core.GetLastStopReason();
}

bool UMicrologicControllerSubsystem::IsConveyorRunning() const
{
	return Core.IsConveyorRunning();
}

bool UMicrologicControllerSubsystem::GetRelayState(int32 RelayNumber) const
{
	return Core.GetRelayPhysical(RelayNumber);
}

bool UMicrologicControllerSubsystem::GetInputState(int32 Channel) const
{
	return Core.GetInputCommitted(Channel);
}

TArray<FMLRelayRuntime> UMicrologicControllerSubsystem::GetRelayStates() const
{
	return Core.GetRelayStates();
}

TArray<FMLInputRuntime> UMicrologicControllerSubsystem::GetInputStates() const
{
	return Core.GetInputStates();
}

TArray<FMLCarState> UMicrologicControllerSubsystem::GetCars() const
{
	return Core.GetCars();
}

TArray<FMLOrder> UMicrologicControllerSubsystem::GetQueue() const
{
	return Core.GetQueue();
}

bool UMicrologicControllerSubsystem::IsRollerSequenceActive() const
{
	return Core.IsRollerSequenceActive();
}

int32 UMicrologicControllerSubsystem::GetRollerCount() const
{
	return Core.GetRollerCount();
}

float UMicrologicControllerSubsystem::GetTimeSinceLastPulse() const
{
	return Core.GetTimeSinceLastPulse();
}

float UMicrologicControllerSubsystem::GetInactivitySeconds() const
{
	return Core.GetInactivitySeconds();
}

// ---------------------------------------------------------------------------
// Factory default configuration
//
// Mirrors a realistic LogicWash V2 site: the standard V2 input-board wiring,
// a 24-relay (3 board) tunnel matching the simulator's machines, and the
// service table used in Micrologic training material.
// ---------------------------------------------------------------------------

FMLControllerConfig UMicrologicControllerSubsystem::MakeFactoryDefaultConfig()
{
	FMLControllerConfig Config;

	Config.Communications.NumInputBoards = 1;
	Config.Communications.NumRelayBoards = 3;
	Config.Communications.bExitDoorEnabled = false;

	Config.Conveyor.InchesPerSecond = 13.94f;
	Config.Conveyor.InchesPerPulse = 12.86f;
	Config.Conveyor.OnActivationService = 20;
	Config.Conveyor.ShutOffService = 21;
	Config.Conveyor.InactivityTimeoutSeconds = 120.f;
	Config.Conveyor.HornTimeSeconds = 3.f;
	Config.Conveyor.HornDelaySeconds = 1.f;

	Config.AntiCollision.RelayNumber = 19;
	Config.AntiCollision.AfterClearsActivateService = 20;
	Config.AntiCollision.SlowDownService = 0;
	Config.AntiCollision.SlowDownTimeSeconds = 3.f;
	Config.AntiCollision.SlowDownHornService = 24;

	Config.RollerDefaults.MinCarLengthFeet = 6.f;
	Config.RollerDefaults.MaxCarLengthFeet = 25.f;
	Config.RollerDefaults.AverageCarLengthFeet = 15.f;
	Config.RollerDefaults.RollerMode = EMLRollerMode::AutomaticRear;
	Config.RollerDefaults.UpFeet = 4.f;
	Config.RollerDefaults.DownFeet = 4.f;
	Config.RollerDefaults.UpAgainFeet = 8.f;
	Config.RollerDefaults.QueueMode = EMLQueueMode::None;
	Config.RollerDefaults.DefaultWashService = 1;

	Config.Security.bRequirePinForRelayOverride = false;

	Config.Sim.TunnelLengthFeet = 120.f;
	Config.Sim.AntiCollisionPadPositionFeet = 110.f;

	// --- Inputs: standard V2 input-board wiring diagram ---
	auto MakeInput = [](int32 Channel, EMLInputType Type, const TCHAR* Description)
	{
		FMLInputConfig Input;
		Input.Channel = Channel;
		Input.Type = Type;
		Input.Description = Description;
		return Input;
	};
	Config.Inputs.Add(MakeInput(1, EMLInputType::None, TEXT("Aux #1")));
	Config.Inputs.Add(MakeInput(2, EMLInputType::Conveyor, TEXT("Conveyor Interlock")));
	Config.Inputs.Add(MakeInput(3, EMLInputType::Entry, TEXT("Lower Entry Sensor (Photo Eyes)")));
	Config.Inputs.Add(MakeInput(4, EMLInputType::None, TEXT("Aux #4")));
	Config.Inputs.Add(MakeInput(5, EMLInputType::TireSwitch, TEXT("Tire Switch")));
	Config.Inputs.Add(MakeInput(6, EMLInputType::UpperEntry, TEXT("Upper Entry Sensor")));
	Config.Inputs.Add(MakeInput(7, EMLInputType::AntiCollision, TEXT("Anti-Collision")));
	Config.Inputs.Add(MakeInput(8, EMLInputType::ExitDoor, TEXT("Exit Door")));
	Config.Inputs.Add(MakeInput(9, EMLInputType::Stall, TEXT("Conveyor Stall")));
	for (int32 Channel = 10; Channel <= 16; ++Channel)
	{
		Config.Inputs.Add(MakeInput(Channel, EMLInputType::None,
			*FString::Printf(TEXT("Aux #%d"), Channel)));
	}

	// --- Relays: a realistic tunnel lineup (positions in feet from the eyes) ---
	auto MakeRelay = [](int32 Number, const TCHAR* Description, EMLRelayType Type, bool bDefault,
	                    EMLFunctionType FnType, float DevicePos, float OnFeet, EMLTurnReference OnRef,
	                    float OffFeet, EMLTurnReference OffRef)
	{
		FMLRelayConfig Relay;
		Relay.RelayNumber = Number;
		Relay.Description = Description;
		Relay.Type = Type;
		Relay.bDefault = bDefault;
		Relay.Function.Type = FnType;
		Relay.Function.DevicePositionFeet = DevicePos;
		Relay.Function.TurnOnFeet = OnFeet;
		Relay.Function.TurnOnReference = OnRef;
		Relay.Function.TurnOffFeet = OffFeet;
		Relay.Function.TurnOffReference = OffRef;
		return Relay;
	};
	using ET = EMLTurnReference;
	using EF = EMLFunctionType;

	Config.Relays.Add(MakeRelay(1, TEXT("Rollers"), EMLRelayType::Roller, false, EF::None, 0, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(2, TEXT("Horn"), EMLRelayType::Horn, false, EF::None, 0, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(3, TEXT("Wetdown Arch"), EMLRelayType::Normal, true, EF::VehicleLength, 5, 2, ET::BeforeFront, 2, ET::AfterRear));
	Config.Relays.Add(MakeRelay(4, TEXT("Presoak"), EMLRelayType::Normal, true, EF::VehicleLength, 12, 1, ET::BeforeFront, 1, ET::AfterRear));
	Config.Relays.Add(MakeRelay(5, TEXT("Tire Applicator (CTA)"), EMLRelayType::Normal, true, EF::AllTires, 15, 1, ET::BeforeFront, 1, ET::AfterFront));

	{
		FMLRelayConfig Wrap = MakeRelay(6, TEXT("Wraps / Side Brushes"), EMLRelayType::Normal, true, EF::VehicleLength, 25, 0, ET::BeforeFront, 0, ET::AfterRear);
		Wrap.InterlockStartSeconds = 2.f;
		FMLModifierConfig MirrorBump;
		MirrorBump.Type = EMLModifierType::MirrorBump;
		MirrorBump.StartFeet = 2.5f;   // retract 2' 6" after the front of the vehicle
		MirrorBump.LengthFeet = 1.75f; // come back in after 1' 9"
		Wrap.Function.Modifiers.Add(MirrorBump);
		Config.Relays.Add(Wrap);
	}
	{
		FMLRelayConfig TopBrush = MakeRelay(7, TEXT("Top Brush"), EMLRelayType::Normal, true, EF::VehicleLength, 32, 0, ET::BeforeFront, 0, ET::AfterRear);
		TopBrush.InterlockStartSeconds = 2.f;
		Config.Relays.Add(TopBrush);
	}
	Config.Relays.Add(MakeRelay(8, TEXT("High Pressure Water"), EMLRelayType::Normal, true, EF::VehicleLength, 40, 1, ET::BeforeFront, 1, ET::AfterRear));
	Config.Relays.Add(MakeRelay(9, TEXT("Triple Foam"), EMLRelayType::Normal, false, EF::VehicleLength, 48, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(10, TEXT("Tire Brushes"), EMLRelayType::Normal, false, EF::VehicleLength, 20, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(11, TEXT("Ceramic Sealant"), EMLRelayType::Normal, false, EF::VehicleLength, 55, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(12, TEXT("Graphene Coat"), EMLRelayType::Normal, false, EF::VehicleLength, 58, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(13, TEXT("Rinse Arch"), EMLRelayType::Normal, true, EF::VehicleLength, 65, 1, ET::BeforeFront, 1, ET::AfterRear));
	Config.Relays.Add(MakeRelay(14, TEXT("Spot Free Rinse"), EMLRelayType::Normal, true, EF::VehicleLength, 70, 1, ET::BeforeFront, 1, ET::AfterRear));
	{
		FMLRelayConfig BuffDry = MakeRelay(15, TEXT("Buff & Dry Cloth"), EMLRelayType::Normal, true, EF::VehicleLength, 78, 0, ET::BeforeFront, 0, ET::AfterRear);
		BuffDry.InterlockStartSeconds = 2.f;
		Config.Relays.Add(BuffDry);
	}
	{
		FMLRelayConfig Blowers = MakeRelay(16, TEXT("Blowers"), EMLRelayType::Normal, true, EF::VehicleLength, 90, 4, ET::BeforeFront, 6, ET::AfterRear);
		Blowers.bInactivityCheck = true;
		Blowers.InterlockStartSeconds = 3.f;
		Blowers.LookBackFeet = 10.f;
		Config.Relays.Add(Blowers);
	}
	Config.Relays.Add(MakeRelay(17, TEXT("Wash Confirmation Light"), EMLRelayType::Normal, false, EF::Light, 0, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(18, TEXT("Tire Shine"), EMLRelayType::Normal, false, EF::AllTires, 74, 1, ET::BeforeFront, 1, ET::AfterFront));

	{
		// Anti-collision ghost relay: Default YES, Type Normal, no wires (cheat sheet).
		FMLRelayConfig AC = MakeRelay(19, TEXT("Anti Collision"), EMLRelayType::Normal, true, EF::VehicleLength, 100, 0, ET::BeforeFront, 0, ET::AfterRear);
		Config.Relays.Add(AC);
	}
	{
		FMLRelayConfig HitchRetract = MakeRelay(20, TEXT("Hitch Retract Solenoid"), EMLRelayType::Normal, false, EF::RearOfVehicle, 78, 3, ET::BeforeRear, 2, ET::AfterRear);
		Config.Relays.Add(HitchRetract);
	}
	Config.Relays.Add(MakeRelay(21, TEXT("Grill Retract Solenoid"), EMLRelayType::Normal, false, EF::FrontOfVehicle, 25, 1, ET::BeforeFront, 2, ET::AfterFront));
	Config.Relays.Add(MakeRelay(22, TEXT("Open Bed Retract Solenoid"), EMLRelayType::Normal, false, EF::RearHalf, 32, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(23, TEXT("Conveyor Start"), EMLRelayType::Normal, false, EF::None, 0, 0, ET::BeforeFront, 0, ET::AfterRear));
	Config.Relays.Add(MakeRelay(24, TEXT("Conveyor Stop"), EMLRelayType::Normal, false, EF::None, 0, 0, ET::BeforeFront, 0, ET::AfterRear));

	// --- Services: the Micrologic training service table ---
	auto MakeService = [](int32 Number, const TCHAR* Description, EMLServiceType Type,
	                      std::initializer_list<int32> Relays)
	{
		FMLServiceConfig Service;
		Service.ServiceNumber = Number;
		Service.Description = Description;
		Service.Type = Type;
		Service.RelayNumbers = Relays;
		return Service;
	};

	Config.Services.Add(MakeService(1, TEXT("Basic Wash"), EMLServiceType::Wash, {}));
	Config.Services.Add(MakeService(2, TEXT("Better Wash"), EMLServiceType::Wash, { 9, 10 }));
	Config.Services.Add(MakeService(3, TEXT("Ceramic Wash"), EMLServiceType::Wash, { 9, 10, 11, 18 }));
	Config.Services.Add(MakeService(4, TEXT("Graphene + Ceramic Wash"), EMLServiceType::Wash, { 9, 10, 11, 12, 18 }));
	Config.Services.Add(MakeService(7, TEXT("Hitch Retract"), EMLServiceType::Service, { 20 }));
	Config.Services.Add(MakeService(8, TEXT("Grill Retract"), EMLServiceType::Service, { 21 }));
	Config.Services.Add(MakeService(9, TEXT("Tire Shine Retract"), EMLServiceType::Deprogrammable, { 18 }));
	Config.Services.Add(MakeService(10, TEXT("Tire Brush Retract"), EMLServiceType::Deprogrammable, { 10 }));
	Config.Services.Add(MakeService(11, TEXT("Full Top Retract"), EMLServiceType::Deprogrammable, { 7 }));
	Config.Services.Add(MakeService(12, TEXT("Buff & Dry Retract"), EMLServiceType::Deprogrammable, { 15 }));
	Config.Services.Add(MakeService(13, TEXT("Wrap Retract"), EMLServiceType::Deprogrammable, { 6 }));
	Config.Services.Add(MakeService(14, TEXT("Open Bed Retract"), EMLServiceType::Service, { 22 }));

	{
		FMLServiceConfig ConveyorStart = MakeService(20, TEXT("Conveyor Start"), EMLServiceType::MomentarilyOn, { 23 });
		ConveyorStart.TimeSeconds = 2.f;
		Config.Services.Add(ConveyorStart);
	}
	{
		FMLServiceConfig ConveyorStop = MakeService(21, TEXT("Conveyor Stop"), EMLServiceType::MomentarilyOn, { 24 });
		ConveyorStop.TimeSeconds = 2.f;
		Config.Services.Add(ConveyorStop);
	}
	{
		FMLServiceConfig Wetdown = MakeService(22, TEXT("Wetdown"), EMLServiceType::MomentarilyOn, { 3 });
		Wetdown.TimeSeconds = 30.f;
		Config.Services.Add(Wetdown);
	}
	{
		FMLServiceConfig DryWetdown = MakeService(23, TEXT("Dry Wetdown (LED Only)"), EMLServiceType::MomentarilyOn, { 17 });
		DryWetdown.TimeSeconds = 30.f;
		Config.Services.Add(DryWetdown);
	}
	{
		FMLServiceConfig HornBlast = MakeService(24, TEXT("Horn Blast"), EMLServiceType::MomentarilyOn, { 2 });
		HornBlast.TimeSeconds = 2.f;
		Config.Services.Add(HornBlast);
	}
	{
		FMLServiceConfig RollerUp = MakeService(25, TEXT("Roller Up"), EMLServiceType::Command, {});
		RollerUp.Command = EMLCommand::RollerRequest;
		Config.Services.Add(RollerUp);
	}

	return Config;
}
