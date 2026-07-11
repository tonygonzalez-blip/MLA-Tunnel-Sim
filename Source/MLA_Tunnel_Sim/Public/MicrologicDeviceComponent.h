// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MicrologicDeviceComponent.generated.h"

class UMicrologicControllerSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMLDeviceSignal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMLDeviceSignalChanged, bool, bOn);

/**
 * Binds a tunnel machine actor to a controller relay.
 *
 * Add this component to a machine Blueprint (Top Brush, Blowers, ...), set
 * RelayNumber to the relay that drives the machine, and hook OnEnergized /
 * OnDeEnergized (or OnSignalChanged) to the machine's start/stop logic.
 * The component mirrors the PHYSICAL relay output — manual board switches
 * included — so machines respond exactly like field equipment.
 */
UCLASS(ClassGroup = (Micrologic), meta = (BlueprintSpawnableComponent))
class MLA_TUNNEL_SIM_API UMicrologicDeviceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMicrologicDeviceComponent();

	/** The relay number on the output boards that powers this device. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 RelayNumber = 0;

	/** Fired when the relay energizes (device power on). */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic")
	FMLDeviceSignal OnEnergized;

	/** Fired when the relay de-energizes (device power off). */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic")
	FMLDeviceSignal OnDeEnergized;

	/** Fired on every relay state change with the new state. */
	UPROPERTY(BlueprintAssignable, Category = "Micrologic")
	FMLDeviceSignalChanged OnSignalChanged;

	UFUNCTION(BlueprintPure, Category = "Micrologic")
	bool IsEnergized() const { return bEnergized; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleRelayStateChanged(int32 InRelayNumber, bool bOn);

	UMicrologicControllerSubsystem* GetController() const;

	bool bEnergized = false;
};
