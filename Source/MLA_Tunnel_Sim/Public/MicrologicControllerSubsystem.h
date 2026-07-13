// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MicrologicTypes.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMLRelayStateChanged, int32, RelayNumber, bool, bOn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMLInputStateChanged, int32, Channel, bool, bOn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLConveyorStateChangedDyn, EMLConveyorState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLConveyorStoppedDyn, EMLStopReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMLSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLCarEventDyn, FMLCarState, Car);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMLServiceExecutedDyn, int32, ServiceNumber, bool, bAccepted);

/**
 * The Micrologic V2 tunnel controller, as a game-instance subsystem.
 *
 * Owns the live simulation core plus a STAGED copy of the configuration that
 * the controller UI edits. Commit / Commit+Reload / Reset / Backup / Restore
 * follow the real controller's Backup-Restore tab semantics.
 *
 * Blueprints reach everything through this class; the tunnel world binds via
 * UMicrologicDeviceComponent / UMicrologicSensorComponent.
 */
UCLASS()
class MLA_TUNNEL_SIM_API UMicrologicControllerSubsystem
	: public UGameInstanceSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// ---- USubsystem ----
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- FTickableGameObject ----
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

	/** Direct access for C++ callers and tests. */
	FMicrologicSimCore& GetCore() { return Core; }
	const FMicrologicSimCore& GetCore() const { return Core; }

	// ---- Login / security -------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Security")
	bool ValidateLogin(const FString& Username, const FString& Password) const;

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Security")
	bool ValidateUiPassword(const FString& Password) const;

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Security")
	bool ValidatePin(const FString& Pin) const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|Security")
	bool IsPinRequiredForRelayOverride() const;

	// ---- Staged configuration (what the UI edits) --------------------------

	UFUNCTION(BlueprintPure, Category = "Micrologic|Config")
	const FMLControllerConfig& GetStagedConfig() const { return StagedConfig; }

	UFUNCTION(BlueprintPure, Category = "Micrologic|Config")
	const FMLControllerConfig& GetLiveConfig() const { return Core.GetConfig(); }

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void SetStagedCommunications(const FMLCommunicationsSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void SetStagedConveyor(const FMLConveyorSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void SetStagedAntiCollision(const FMLAntiCollisionSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void SetStagedRollerDefaults(const FMLRollerDefaults& Settings);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void SetStagedSecurity(const FMLSecuritySettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void SetStagedSim(const FMLSimSettings& Settings);

	/** Adds or replaces (by Channel) an input row in the staged config. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void UpsertStagedInput(const FMLInputConfig& Input);

	/** Adds or replaces (by RelayNumber) a relay row in the staged config. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void UpsertStagedRelay(const FMLRelayConfig& Relay);

	/** Adds or replaces (by ServiceNumber) a service row in the staged config. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void UpsertStagedService(const FMLServiceConfig& Service);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void RemoveStagedRelay(int32 RelayNumber);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void RemoveStagedService(int32 ServiceNumber);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void RemoveStagedInput(int32 Channel);

	// ---- Backup/Restore tab -------------------------------------------------

	/** Commit: apply staged changes to the live controller without reloading (test before writing). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void Commit();

	/** Commit + Reload: apply staged changes and reload the controller (queue is kept). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void CommitAndReload();

	/** Reset: erase the configuration back to factory defaults (staged and live). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void ResetConfiguration();

	/** Discard staged edits, re-copying the live configuration. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	void RevertStaged();

	/** Backup: write the LIVE configuration to an XML file. Returns the full path ("" on failure). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	FString BackupToXmlFile(const FString& BackupName);

	/** Restore: load an XML backup into the STAGED config (Commit+Reload applies it). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	bool RestoreFromXmlFile(const FString& FilePath);

	/** List existing backup files (full paths, newest first). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	TArray<FString> ListBackupFiles() const;

	/** The live configuration serialized to XML (Backup file contents). */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Config")
	FString GetLiveConfigAsXml() const;

	// ---- Field wiring / hardware -------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Hardware")
	void SetInputRaw(int32 Channel, bool bState);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Hardware")
	void SetInputRawByType(EMLInputType Type, bool bState);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Hardware")
	void FlagMeasuringCarOpenBed(bool bOpenBed);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Hardware")
	void PressStartCircuit(int32 Circuit);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Hardware")
	void SetStopCircuit(int32 Circuit, bool bClosed);

	UFUNCTION(BlueprintPure, Category = "Micrologic|Hardware")
	bool IsStopCircuitClosed(int32 Circuit) const;

	/**
	 * Flip a relay board's 3-position manual switch. When a PIN is required,
	 * pass the PIN entered by the user; wrong PIN = rejected (returns false).
	 */
	UFUNCTION(BlueprintCallable, Category = "Micrologic|Hardware")
	bool SetRelayOverride(int32 RelayNumber, EMLRelayOverride Override, const FString& Pin);

	UFUNCTION(BlueprintPure, Category = "Micrologic|Hardware")
	EMLRelayOverride GetRelayOverride(int32 RelayNumber) const;

	// ---- Orders / services / rollers ----------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Controller")
	int32 SendOrder(const TArray<int32>& ServiceNumbers);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Controller")
	bool ExecuteService(int32 ServiceNumber);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Controller")
	bool ExecuteCommand(EMLCommand Command);

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Controller")
	bool RequestRoller();

	UFUNCTION(BlueprintCallable, Category = "Micrologic|Controller")
	void AbortRoller();

	// ---- State queries (dashboard / boards / world) ---------------------------

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	EMLConveyorState GetConveyorState() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	EMLStopReason GetLastStopReason() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	bool IsConveyorRunning() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	bool GetRelayState(int32 RelayNumber) const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	bool GetInputState(int32 Channel) const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	TArray<FMLRelayRuntime> GetRelayStates() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	TArray<FMLInputRuntime> GetInputStates() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	TArray<FMLCarState> GetCars() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	TArray<FMLOrder> GetQueue() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	bool IsRollerSequenceActive() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	int32 GetRollerCount() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	float GetTimeSinceLastPulse() const;

	UFUNCTION(BlueprintPure, Category = "Micrologic|State")
	float GetInactivitySeconds() const;

	// ---- Events ---------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLRelayStateChanged OnRelayStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLInputStateChanged OnInputStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLConveyorStateChangedDyn OnConveyorStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLConveyorStoppedDyn OnConveyorStopped;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLSimpleEvent OnPulse;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLSimpleEvent OnQueueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLSimpleEvent OnConfigChanged;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLCarEventDyn OnCarEntered;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLCarEventDyn OnCarMeasured;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLCarEventDyn OnCarExited;

	UPROPERTY(BlueprintAssignable, Category = "Micrologic|Events")
	FMLServiceExecutedDyn OnServiceExecuted;

	/** The factory-default configuration shipped with the simulator. */
	static FMLControllerConfig MakeFactoryDefaultConfig();

private:
	FMicrologicSimCore Core;
	FMLControllerConfig StagedConfig;

	void BindCoreEvents();
	void SaveConfigToSlot() const;
	bool LoadConfigFromSlot(FMLControllerConfig& OutConfig) const;
	FString BackupDirectory() const;

	static const TCHAR* SaveSlotName;
};
