// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MicrologicTypes.h"
#include "MicrologicSensorComponent.generated.h"

class UMicrologicControllerSubsystem;

/**
 * Binds a field sensor actor (photo eyes, tire switch, anti-collision pad,
 * exit-door switch, stall sensor...) to a controller input channel.
 *
 * Add to the sensor Blueprint and call SetTriggered from its overlap/trace
 * logic. Wire either by explicit Channel, or by InputType (resolved against
 * the live config at the time of the call — survives re-wiring in the UI).
 */
UCLASS(ClassGroup = (Micrologic), meta = (BlueprintSpawnableComponent))
class MLA_TUNNEL_SIM_API UMicrologicSensorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMicrologicSensorComponent();

	/** Explicit input channel (1-based). 0 = resolve by InputType instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 Channel = 0;

	/** Used when Channel == 0: the first input configured with this type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLInputType InputType = EMLInputType::None;

	/** Set the raw wire state of this sensor's input. */
	UFUNCTION(BlueprintCallable, Category = "Micrologic")
	void SetTriggered(bool bTriggered);

	UFUNCTION(BlueprintPure, Category = "Micrologic")
	bool IsTriggered() const { return bLastState; }

private:
	UMicrologicControllerSubsystem* GetController() const;

	bool bLastState = false;
};
