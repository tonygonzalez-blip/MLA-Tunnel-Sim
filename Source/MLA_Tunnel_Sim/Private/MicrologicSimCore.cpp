// Copyright Micrologic Associates. All Rights Reserved.

#include "MicrologicSimCore.h"

namespace
{
	constexpr float KMinPulseIntervalFeet = 0.01f; // guard against zero/negative IPP
	constexpr int32 KMaxMacroDepth = 4;

	float SignedOffset(EMLTurnReference Ref, float OffsetFeet)
	{
		switch (Ref)
		{
		case EMLTurnReference::BeforeFront:
		case EMLTurnReference::BeforeRear:
			return -OffsetFeet;
		case EMLTurnReference::AfterFront:
		case EMLTurnReference::AfterRear:
		default:
			return OffsetFeet;
		}
	}

	bool RefUsesFront(EMLTurnReference Ref)
	{
		return Ref == EMLTurnReference::BeforeFront || Ref == EMLTurnReference::AfterFront;
	}
}

FMicrologicSimCore::FMicrologicSimCore()
{
	SetConfig(FMLControllerConfig(), /*bReload=*/true);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void FMicrologicSimCore::SetConfig(const FMLControllerConfig& InConfig, bool bReload)
{
	Config = InConfig;

	const int32 NumInputs = FMath::Max(1, Config.Communications.NumInputBoards) * 16;
	const int32 NumRelays = FMath::Max(1, Config.Communications.NumRelayBoards) * 8;

	// Preserve raw wire states and manual switch positions across config swaps —
	// rewiring a controller does not move the physical switches.
	Inputs.SetNum(NumInputs);
	Relays.SetNum(NumRelays);

	if (bReload)
	{
		ResetRuntime();
	}
	else
	{
		// Plain Commit: re-run debounce/inversion against the new config.
		for (int32 Channel = 1; Channel <= Inputs.Num(); ++Channel)
		{
			SetInputRaw(Channel, Inputs[Channel - 1].bRaw);
		}
		EvaluateRelays();
	}
}

void FMicrologicSimCore::ResetRuntime()
{
	// Commit + Reload keeps the controller queue (per the cheat sheet), so the
	// queue is intentionally NOT cleared here. Use RemoveAllOrders for that.
	Cars_Internal.Empty();
	Cars.Empty();
	Momentaries.Empty();
	ToggledRelays.Empty();
	MeasuringCarIndex = INDEX_NONE;

	ConveyorState = EMLConveyorState::Stopped;
	LastStopReason = EMLStopReason::None;
	ConveyorPhaseTimer = 0.f;
	ConveyorRunSeconds = 0.f;
	ConveyorStopSeconds = 0.f;
	SlowDownRemaining = 0.f;
	bAntiCollisionHold = false;
	bACTripped = false;

	PulseAccumulatorFeet = 0.f;
	TimeSinceLastPulse = 0.f;
	TotalPulses = 0;
	InactivitySeconds = 0.f;

	RollerSequence = FRollerSequence();
	RollerCount = 0;
	LastTireSwitchLength = -1.f;

	for (FInputRuntime& Input : Inputs)
	{
		Input.PendingTimer = -1.f;
		// Recompute effective/committed from raw against (possibly new) inversion flags.
	}
	for (int32 Channel = 1; Channel <= Inputs.Num(); ++Channel)
	{
		FInputRuntime& Input = Inputs[Channel - 1];
		const FMLInputConfig* Cfg = FindInputConfig(Channel);
		const bool bEffective = Cfg && Cfg->bInverted ? !Input.bRaw : Input.bRaw;
		Input.bEffective = bEffective;
		Input.bCommitted = bEffective;
	}

	for (FRelayRuntime& Relay : Relays)
	{
		Relay.bLogicalOn = false;
		Relay.bWindowActive = false;
		Relay.OffPendingTimer = -1.f;
		// Manual overrides survive a reload; physical state re-derived below.
	}

	SyncConveyorInterlockWire();
	EvaluateRelays();
}

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------

void FMicrologicSimCore::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	UpdateInputsDebounce(DeltaSeconds);
	UpdateMomentaries(DeltaSeconds);
	UpdateConveyor(DeltaSeconds);

	TimeSinceLastPulse += DeltaSeconds;

	if (IsConveyorRunning())
	{
		PulseAccumulatorFeet += (Config.Conveyor.InchesPerSecond / 12.f) * DeltaSeconds;
		const float Interval = PulseFeetInterval();
		while (PulseAccumulatorFeet >= Interval)
		{
			PulseAccumulatorFeet -= Interval;
			EmitPulse();
		}
	}

	UpdateAntiCollision(DeltaSeconds);
	UpdateInactivity(DeltaSeconds);
	EvaluateRelays();
}

float FMicrologicSimCore::PulseFeetInterval() const
{
	return FMath::Max(KMinPulseIntervalFeet, Config.Conveyor.InchesPerPulse / 12.f);
}

// ---------------------------------------------------------------------------
// Pulses & cars
// ---------------------------------------------------------------------------

void FMicrologicSimCore::EmitPulse()
{
	++TotalPulses;
	TimeSinceLastPulse = 0.f;

	const float PulseFeet = PulseFeetInterval();
	AdvanceCarsOnePulse(PulseFeet);
	UpdateRollerSequenceOnPulse(PulseFeet);

	OnPulse.Broadcast();
}

void FMicrologicSimCore::AdvanceCarsOnePulse(float PulseFeet)
{
	bool bChanged = false;

	for (int32 Index = 0; Index < Cars_Internal.Num(); ++Index)
	{
		FCar& Car = Cars_Internal[Index];
		Car.State.FrontPositionFeet += PulseFeet;

		if (Car.State.bMeasuring)
		{
			Car.State.LengthFeet += PulseFeet;
			if (Car.State.LengthFeet >= Config.RollerDefaults.MaxCarLengthFeet)
			{
				// Max Car Length reached: complete the order and stop measuring.
				Car.State.LengthFeet = Config.RollerDefaults.MaxCarLengthFeet;
				FinishCarMeasurement(/*bValid=*/true);
			}
		}
		bChanged = true;
	}

	// Despawn cars that have fully exited the tunnel.
	for (int32 Index = Cars_Internal.Num() - 1; Index >= 0; --Index)
	{
		const FCar& Car = Cars_Internal[Index];
		if (!Car.State.bMeasuring &&
			Car.State.RearPositionFeet() > Config.Sim.TunnelLengthFeet)
		{
			const FMLCarState Exited = Car.State;
			Cars_Internal.RemoveAt(Index);
			if (MeasuringCarIndex > Index)
			{
				--MeasuringCarIndex;
			}
			RebuildPublicCarStates();
			OnCarExited.Broadcast(Exited);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		RebuildPublicCarStates();
	}
}

void FMicrologicSimCore::BeginCarMeasurement()
{
	if (MeasuringCarIndex != INDEX_NONE || !CanEngageWash())
	{
		return;
	}

	FCar Car;
	Car.State.CarId = NextCarId++;
	Car.State.FrontPositionFeet = 0.f;
	Car.State.LengthFeet = 0.f;
	Car.State.bMeasuring = true;
	Cars_Internal.Add(Car);
	MeasuringCarIndex = Cars_Internal.Num() - 1;
	LastTireSwitchLength = -1.f;

	// The order binds the moment the car breaks the eyes ("Default Wash if
	// None Programmed" fires if no order was sent PRIOR), so devices close to
	// the entrance fire while the car is still being measured. If this turns
	// out to be a blip shorter than Minimum Car Length, the order goes back.
	FCar& NewCar = Cars_Internal.Last();
	if (Queue.Num() > 0)
	{
		NewCar.BoundOrder = Queue[0];
		NewCar.bHasBoundOrder = true;
		Queue.RemoveAt(0);
		ApplyOrderToCar(NewCar, NewCar.BoundOrder);
		OnQueueChanged.Broadcast();
	}
	else if (Config.RollerDefaults.DefaultWashService != 0)
	{
		FMLOrder Fallback;
		Fallback.ServiceNumbers.Add(Config.RollerDefaults.DefaultWashService);
		ApplyOrderToCar(NewCar, Fallback);
	}
	else
	{
		// No order and no default: the car rides through with default relays only.
		ApplyOrderToCar(NewCar, FMLOrder());
	}

	RebuildPublicCarStates();
	OnCarEntered.Broadcast(Cars_Internal.Last().State);
}

void FMicrologicSimCore::FinishCarMeasurement(bool bValid)
{
	if (MeasuringCarIndex == INDEX_NONE || !Cars_Internal.IsValidIndex(MeasuringCarIndex))
	{
		MeasuringCarIndex = INDEX_NONE;
		return;
	}

	FCar& Car = Cars_Internal[MeasuringCarIndex];
	Car.State.bMeasuring = false;

	if (!bValid || Car.State.LengthFeet < Config.RollerDefaults.MinCarLengthFeet)
	{
		// Below Minimum Car Length: the photo eyes never saw a real car.
		// An order consumed at eyes-break goes back to the head of the queue.
		if (Car.bHasBoundOrder)
		{
			Queue.Insert(Car.BoundOrder, 0);
			OnQueueChanged.Broadcast();
		}
		Cars_Internal.RemoveAt(MeasuringCarIndex);
		MeasuringCarIndex = INDEX_NONE;
		RebuildPublicCarStates();
		return;
	}

	MeasuringCarIndex = INDEX_NONE;
	RebuildPublicCarStates();
	OnCarMeasured.Broadcast(Car.State);
}

void FMicrologicSimCore::CollectOrderServices(const TArray<int32>& ServiceNumbers, int32 Depth,
                                              TArray<const FMLServiceConfig*>& OutServices) const
{
	if (Depth >= KMaxMacroDepth)
	{
		return;
	}
	for (const int32 ServiceNumber : ServiceNumbers)
	{
		if (const FMLServiceConfig* Service = FindServiceConfig(ServiceNumber))
		{
			if (Service->Type == EMLServiceType::Macro)
			{
				CollectOrderServices(Service->MacroServiceNumbers, Depth + 1, OutServices);
			}
			else
			{
				OutServices.Add(Service);
			}
		}
	}
}

void FMicrologicSimCore::ApplyOrderToCar(FCar& Car, const FMLOrder& Order)
{
	// Default relays fire for every wash.
	for (const FMLRelayConfig& Relay : Config.Relays)
	{
		if (Relay.bActive && Relay.bDefault)
		{
			Car.Program.ProgrammedRelays.Add(Relay.RelayNumber);
		}
	}

	// Expand macros, then apply in two phases so De-programmable removals win
	// "no matter the wash package" — regardless of order within the order.
	TArray<const FMLServiceConfig*> Services;
	CollectOrderServices(Order.ServiceNumbers, 0, Services);

	for (const FMLServiceConfig* Service : Services)
	{
		if (Service->Type != EMLServiceType::Deprogrammable)
		{
			ApplyServiceToCar(Car, *Service);
		}
	}
	for (const FMLServiceConfig* Service : Services)
	{
		if (Service->Type == EMLServiceType::Deprogrammable)
		{
			ApplyServiceToCar(Car, *Service);
		}
	}

	for (const int32 ServiceNumber : Order.ServiceNumbers)
	{
		if (FindServiceConfig(ServiceNumber))
		{
			Car.State.ServiceNumbers.Add(ServiceNumber);
		}
	}
}

void FMicrologicSimCore::ApplyServiceToCar(FCar& Car, const FMLServiceConfig& Service)
{
	switch (Service.Type)
	{
	case EMLServiceType::Wash:
	case EMLServiceType::Service:
		for (const int32 Relay : Service.RelayNumbers)
		{
			Car.Program.ProgrammedRelays.Add(Relay);
		}
		break;

	case EMLServiceType::Deprogrammable:
		for (const int32 Relay : Service.RelayNumbers)
		{
			Car.Program.ProgrammedRelays.Remove(Relay);
		}
		break;

	case EMLServiceType::TurnOff:
		// Turns off the selected relay and everything after it for this wash.
		for (const int32 Relay : Service.RelayNumbers)
		{
			if (Car.Program.CascadeOffFromRelay == 0 || Relay < Car.Program.CascadeOffFromRelay)
			{
				Car.Program.CascadeOffFromRelay = Relay;
			}
		}
		break;

	case EMLServiceType::TurnOn:
		for (const int32 Relay : Service.RelayNumbers)
		{
			if (Car.Program.CascadeOnFromRelay == 0 || Relay < Car.Program.CascadeOnFromRelay)
			{
				Car.Program.CascadeOnFromRelay = Relay;
			}
		}
		break;

	case EMLServiceType::Override:
		for (const int32 Relay : Service.RelayNumbers)
		{
			Car.Program.OverrideRelays.Add(Relay);
			Car.Program.ProgrammedRelays.Add(Relay);
		}
		break;

	case EMLServiceType::Macro:
		if (MacroDepth < KMaxMacroDepth)
		{
			++MacroDepth;
			for (const int32 Sub : Service.MacroServiceNumbers)
			{
				if (const FMLServiceConfig* SubService = FindServiceConfig(Sub))
				{
					ApplyServiceToCar(Car, *SubService);
				}
			}
			--MacroDepth;
		}
		break;

	// Global service types behave the same whether or not they arrive in an order.
	case EMLServiceType::MomentarilyOn:
	case EMLServiceType::MomentarilyOff:
	case EMLServiceType::Toggler:
	case EMLServiceType::Command:
	default:
		ExecuteService(Service.ServiceNumber);
		break;
	}
}

void FMicrologicSimCore::RebuildPublicCarStates()
{
	Cars.Reset(Cars_Internal.Num());
	for (const FCar& Car : Cars_Internal)
	{
		FMLCarState State = Car.State;
		State.ProgrammedRelays = Car.Program.ProgrammedRelays.Array();
		State.OverrideRelays = Car.Program.OverrideRelays.Array();
		Cars.Add(MoveTemp(State));
	}
}

// ---------------------------------------------------------------------------
// Inputs
// ---------------------------------------------------------------------------

void FMicrologicSimCore::SetInputRaw(int32 Channel, bool bState)
{
	if (Channel < 1 || Channel > Inputs.Num())
	{
		return;
	}

	FInputRuntime& Input = Inputs[Channel - 1];
	Input.bRaw = bState;

	const FMLInputConfig* Cfg = FindInputConfig(Channel);
	const bool bEffective = (Cfg && Cfg->bInverted) ? !bState : bState;
	Input.bEffective = bEffective;

	if (bEffective == Input.bCommitted)
	{
		Input.PendingTimer = -1.f; // change reverted before debounce elapsed
		return;
	}

	const float Debounce = Cfg ? EffectiveDebounce(*Cfg) : Config.RollerDefaults.DefaultInputDebounceSeconds;
	if (Debounce <= 0.f)
	{
		Input.PendingTimer = -1.f;
		Input.bCommitted = bEffective;
		OnInputCommitted(Channel, bEffective);
	}
	else
	{
		Input.bPendingState = bEffective;
		Input.PendingTimer = Debounce;
	}
}

void FMicrologicSimCore::SetInputRawByType(EMLInputType Type, bool bState)
{
	const int32 Channel = FindChannelByType(Type);
	if (Channel != INDEX_NONE)
	{
		SetInputRaw(Channel, bState);
	}
}

void FMicrologicSimCore::FlagMeasuringCarOpenBed(bool bOpenBed)
{
	if (MeasuringCarIndex != INDEX_NONE && Cars_Internal.IsValidIndex(MeasuringCarIndex))
	{
		Cars_Internal[MeasuringCarIndex].State.bOpenPickupBed = bOpenBed;
		RebuildPublicCarStates();
	}
}

void FMicrologicSimCore::UpdateInputsDebounce(float Dt)
{
	for (int32 Channel = 1; Channel <= Inputs.Num(); ++Channel)
	{
		FInputRuntime& Input = Inputs[Channel - 1];
		if (Input.PendingTimer >= 0.f)
		{
			Input.PendingTimer -= Dt;
			if (Input.PendingTimer <= 0.f)
			{
				Input.PendingTimer = -1.f;
				if (Input.bCommitted != Input.bPendingState)
				{
					Input.bCommitted = Input.bPendingState;
					OnInputCommitted(Channel, Input.bCommitted);
				}
			}
		}
	}
}

void FMicrologicSimCore::OnInputCommitted(int32 Channel, bool bNewState)
{
	OnInputChanged.Broadcast(Channel, bNewState);

	const FMLInputConfig* Cfg = FindInputConfig(Channel);
	if (!Cfg)
	{
		return;
	}

	switch (Cfg->Type)
	{
	case EMLInputType::Entry:
		if (bNewState)
		{
			BeginCarMeasurement();
		}
		else if (MeasuringCarIndex != INDEX_NONE)
		{
			FinishCarMeasurement(/*bValid=*/true);
		}
		break;

	case EMLInputType::TireSwitch:
		if (bNewState && MeasuringCarIndex != INDEX_NONE && Cars_Internal.IsValidIndex(MeasuringCarIndex))
		{
			FCar& Car = Cars_Internal[MeasuringCarIndex];
			Car.State.TireOffsetsFeet.Add(Car.State.LengthFeet);
			LastTireSwitchLength = Car.State.LengthFeet;
			RebuildPublicCarStates();
		}
		break;

	case EMLInputType::UpperEntry:
		// Falling edge while the entry eyes are still engaged = end of cab
		// (truck bed present). Only latched once per vehicle.
		if (!bNewState && MeasuringCarIndex != INDEX_NONE && Cars_Internal.IsValidIndex(MeasuringCarIndex))
		{
			FCar& Car = Cars_Internal[MeasuringCarIndex];
			if (!Car.bUpperEntrySeenOff && HasCommittedInput(EMLInputType::Entry))
			{
				Car.bUpperEntrySeenOff = true;
				Car.State.CabLengthFeet = Car.State.LengthFeet;
				RebuildPublicCarStates();
			}
		}
		break;

	case EMLInputType::RollerPosition:
		if (bNewState)
		{
			++RollerCount;
		}
		break;

	case EMLInputType::Stall:
		if (bNewState)
		{
			RequestConveyorStop(EMLStopReason::Stall);
		}
		break;

	case EMLInputType::ExitDoor:
		if (!bNewState && Config.Communications.bExitDoorEnabled &&
			Config.Conveyor.ShutOffService != 0 && IsConveyorRunning())
		{
			RequestConveyorStop(EMLStopReason::ExitDoor);
		}
		break;

	case EMLInputType::Trigger:
		if (bNewState && Cfg->TriggerServiceNumber != 0)
		{
			ExecuteService(Cfg->TriggerServiceNumber);
		}
		break;

	case EMLInputType::AntiCollision:
	case EMLInputType::Conveyor:
	case EMLInputType::None:
	default:
		break; // handled by polling logic / informational
	}

	EvaluateRelays();
}

// ---------------------------------------------------------------------------
// Start/Stop board
// ---------------------------------------------------------------------------

void FMicrologicSimCore::PressStartCircuit(int32 Circuit)
{
	if (Circuit >= 1 && Circuit <= 5)
	{
		RequestConveyorStart();
	}
}

void FMicrologicSimCore::SetStopCircuit(int32 Circuit, bool bClosed)
{
	if (Circuit < 1 || Circuit > 5)
	{
		return;
	}
	StopCircuits[Circuit - 1] = bClosed;
	if (!bClosed)
	{
		// Normally-Closed circuit opened: conveyor shuts off immediately.
		RequestConveyorStop(EMLStopReason::StopCircuit);
	}
}

bool FMicrologicSimCore::IsStopCircuitClosed(int32 Circuit) const
{
	return (Circuit >= 1 && Circuit <= 5) ? StopCircuits[Circuit - 1] : true;
}

// ---------------------------------------------------------------------------
// Orders / queue
// ---------------------------------------------------------------------------

int32 FMicrologicSimCore::SendOrder(const TArray<int32>& ServiceNumbers)
{
	FMLOrder Order;
	Order.OrderId = NextOrderId++;
	Order.ServiceNumbers = ServiceNumbers;

	switch (Config.RollerDefaults.QueueMode)
	{
	case EMLQueueMode::None:
		// Only one order may wait; a second order replaces the first.
		Queue.Reset();
		Queue.Add(Order);
		break;

	case EMLQueueMode::Random:
	case EMLQueueMode::Sequential:
		Queue.Add(Order);
		break;
	}

	if (Config.RollerDefaults.ServiceOnQueued != 0)
	{
		ExecuteService(Config.RollerDefaults.ServiceOnQueued);
	}

	// A conveyor stopped for inactivity restarts when work arrives.
	if (ConveyorState == EMLConveyorState::Stopped &&
		LastStopReason == EMLStopReason::Inactivity &&
		Config.Conveyor.OnActivationService != 0)
	{
		ExecuteService(Config.Conveyor.OnActivationService);
	}

	OnQueueChanged.Broadcast();
	return Order.OrderId;
}

void FMicrologicSimCore::CancelOrder()
{
	if (Queue.Num() > 0)
	{
		Queue.Pop();
		OnQueueChanged.Broadcast();
	}
}

void FMicrologicSimCore::RemoveLastOrder()
{
	CancelOrder();
}

void FMicrologicSimCore::RemoveAllOrders()
{
	if (Queue.Num() > 0)
	{
		Queue.Reset();
		OnQueueChanged.Broadcast();
	}
}

// ---------------------------------------------------------------------------
// Services & commands
// ---------------------------------------------------------------------------

bool FMicrologicSimCore::ExecuteService(int32 ServiceNumber)
{
	const FMLServiceConfig* Service = FindServiceConfig(ServiceNumber);
	if (!Service)
	{
		OnServiceExecuted.Broadcast(ServiceNumber, false);
		return false;
	}

	bool bAccepted = true;

	switch (Service->Type)
	{
	case EMLServiceType::MomentarilyOn:
	case EMLServiceType::MomentarilyOff:
		for (const int32 Relay : Service->RelayNumbers)
		{
			FMomentaryAction Action;
			Action.RelayNumber = Relay;
			Action.bTurnOn = (Service->Type == EMLServiceType::MomentarilyOn);
			Action.DelayRemaining = FMath::Max(0.f, Service->DelaySeconds);
			Action.DurationRemaining = FMath::Max(0.f, Service->TimeSeconds);
			Momentaries.Add(Action);
		}
		break;

	case EMLServiceType::Toggler:
		for (const int32 Relay : Service->RelayNumbers)
		{
			if (ToggledRelays.Contains(Relay))
			{
				ToggledRelays.Remove(Relay);
			}
			else
			{
				ToggledRelays.Add(Relay);
			}
		}
		break;

	case EMLServiceType::Macro:
		if (MacroDepth < KMaxMacroDepth)
		{
			++MacroDepth;
			for (const int32 Sub : Service->MacroServiceNumbers)
			{
				ExecuteService(Sub);
			}
			--MacroDepth;
		}
		break;

	case EMLServiceType::Command:
		bAccepted = ExecuteCommand(Service->Command);
		break;

	// Car-programming service types executed directly (native UI / PBS override
	// path) apply to the newest vehicle in the tunnel.
	case EMLServiceType::Wash:
	case EMLServiceType::Service:
	case EMLServiceType::Deprogrammable:
	case EMLServiceType::TurnOff:
	case EMLServiceType::TurnOn:
	case EMLServiceType::Override:
		if (Cars_Internal.Num() > 0)
		{
			ApplyServiceToCar(Cars_Internal.Last(), *Service);
			Cars_Internal.Last().State.ServiceNumbers.Add(ServiceNumber);
			RebuildPublicCarStates();
		}
		else
		{
			bAccepted = false;
		}
		break;

	default:
		bAccepted = false;
		break;
	}

	// The Conveyor tab designates services as the activation / shut-off buttons.
	if (ServiceNumber == Config.Conveyor.OnActivationService && Config.Conveyor.OnActivationService != 0)
	{
		RequestConveyorStart();
	}
	if (ServiceNumber == Config.Conveyor.ShutOffService && Config.Conveyor.ShutOffService != 0)
	{
		RequestConveyorStop(EMLStopReason::Manual);
	}

	EvaluateRelays();
	OnServiceExecuted.Broadcast(ServiceNumber, bAccepted);
	return bAccepted;
}

bool FMicrologicSimCore::ExecuteCommand(EMLCommand Command)
{
	switch (Command)
	{
	case EMLCommand::CancelOrder:
		CancelOrder();
		return true;
	case EMLCommand::RemoveLast:
		RemoveLastOrder();
		return true;
	case EMLCommand::RemoveAll:
		RemoveAllOrders();
		return true;
	case EMLCommand::RollerRequest:
		return RequestRoller();
	case EMLCommand::RollerAbort:
		AbortRoller();
		return true;
	case EMLCommand::AcceptOrder:
		// Order assembly happens at the sending station (PBS widget / VQ);
		// the assembled order arrives via SendOrder.
		return true;
	case EMLCommand::None:
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// Rollers
// ---------------------------------------------------------------------------

bool FMicrologicSimCore::RequestRoller()
{
	if (RollerSequence.bActive)
	{
		return false;
	}

	if (Config.RollerDefaults.bNeedsCarQueuedForRollerRequest && Queue.Num() == 0)
	{
		return false;
	}

	if (Config.RollerDefaults.RollerMode == EMLRollerMode::ManualFront)
	{
		// Manual/Front listens to the tire switch: without a tire switch input
		// defined, the controller will not fire a series of rollers.
		if (FindChannelByType(EMLInputType::TireSwitch) == INDEX_NONE)
		{
			return false;
		}
	}

	RollerSequence.bActive = true;
	RollerSequence.Phase = 0;
	RollerSequence.DistanceRemaining = FMath::Max(0.f, Config.RollerDefaults.UpFeet);
	EvaluateRelays();
	return true;
}

void FMicrologicSimCore::AbortRoller()
{
	if (RollerSequence.bActive)
	{
		RollerSequence = FRollerSequence();
		EvaluateRelays();
	}
}

void FMicrologicSimCore::UpdateRollerSequenceOnPulse(float PulseFeet)
{
	if (!RollerSequence.bActive)
	{
		return;
	}

	RollerSequence.DistanceRemaining -= PulseFeet;
	while (RollerSequence.bActive && RollerSequence.DistanceRemaining <= 0.f)
	{
		const float Carry = -RollerSequence.DistanceRemaining;
		switch (RollerSequence.Phase)
		{
		case 0: // Up finished -> Down (hold)
			RollerSequence.Phase = 1;
			RollerSequence.DistanceRemaining = FMath::Max(0.f, Config.RollerDefaults.DownFeet) - Carry;
			break;
		case 1: // Down finished -> Up Again
			RollerSequence.Phase = 2;
			RollerSequence.DistanceRemaining = FMath::Max(0.f, Config.RollerDefaults.UpAgainFeet) - Carry;
			break;
		case 2: // Up Again finished -> done
		default:
			RollerSequence = FRollerSequence();
			break;
		}
	}
}

// ---------------------------------------------------------------------------
// Conveyor
// ---------------------------------------------------------------------------

void FMicrologicSimCore::RequestConveyorStart()
{
	if (ConveyorState != EMLConveyorState::Stopped)
	{
		return;
	}

	// Safety gates.
	for (int32 Circuit = 1; Circuit <= 5; ++Circuit)
	{
		if (!IsStopCircuitClosed(Circuit))
		{
			return;
		}
	}
	if (HasCommittedInput(EMLInputType::Stall))
	{
		return;
	}
	if (Config.Communications.bExitDoorEnabled && Config.Conveyor.ShutOffService != 0)
	{
		const int32 ExitDoorChannel = FindChannelByType(EMLInputType::ExitDoor);
		if (ExitDoorChannel != INDEX_NONE && !GetInputCommitted(ExitDoorChannel))
		{
			return;
		}
	}
	if (Config.AntiCollision.RelayNumber != 0 && HasCommittedInput(EMLInputType::AntiCollision))
	{
		// A vehicle is stopped at the end of the tunnel; wait for it to clear.
		return;
	}

	LastStopReason = EMLStopReason::None;

	if (Config.Conveyor.HornDelaySeconds > 0.f)
	{
		ConveyorPhaseTimer = Config.Conveyor.HornDelaySeconds;
		SetConveyorState(EMLConveyorState::HornDelay);
	}
	else if (Config.Conveyor.HornTimeSeconds > 0.f)
	{
		ConveyorPhaseTimer = Config.Conveyor.HornTimeSeconds;
		SetConveyorState(EMLConveyorState::Horn);
	}
	else
	{
		SetConveyorState(EMLConveyorState::Running);
	}
}

void FMicrologicSimCore::RequestConveyorStop(EMLStopReason Reason)
{
	if (ConveyorState == EMLConveyorState::Stopped)
	{
		return;
	}
	LastStopReason = Reason;
	if (Reason == EMLStopReason::AntiCollision)
	{
		bAntiCollisionHold = true;
	}
	SlowDownRemaining = 0.f;
	SetConveyorState(EMLConveyorState::Stopped);
	OnConveyorStopped.Broadcast(Reason);
}

void FMicrologicSimCore::SetConveyorState(EMLConveyorState NewState)
{
	if (ConveyorState == NewState)
	{
		return;
	}
	ConveyorState = NewState;
	if (NewState == EMLConveyorState::Stopped)
	{
		ConveyorRunSeconds = 0.f;
	}
	SyncConveyorInterlockWire();

	// A car already sitting on the entry eyes starts measuring the moment the
	// conveyor (and its interlock input) comes up.
	if (NewState == EMLConveyorState::Running &&
		MeasuringCarIndex == INDEX_NONE && HasCommittedInput(EMLInputType::Entry))
	{
		BeginCarMeasurement();
	}

	OnConveyorStateChanged.Broadcast(NewState);
}

void FMicrologicSimCore::UpdateConveyor(float Dt)
{
	switch (ConveyorState)
	{
	case EMLConveyorState::HornDelay:
		ConveyorPhaseTimer -= Dt;
		if (ConveyorPhaseTimer <= 0.f)
		{
			if (Config.Conveyor.HornTimeSeconds > 0.f)
			{
				ConveyorPhaseTimer = Config.Conveyor.HornTimeSeconds;
				SetConveyorState(EMLConveyorState::Horn);
			}
			else
			{
				SetConveyorState(EMLConveyorState::Running);
			}
		}
		break;

	case EMLConveyorState::Horn:
		ConveyorPhaseTimer -= Dt;
		if (ConveyorPhaseTimer <= 0.f)
		{
			SetConveyorState(EMLConveyorState::Running);
		}
		break;

	case EMLConveyorState::Running:
	case EMLConveyorState::SlowingDown:
		ConveyorRunSeconds += Dt;
		ConveyorStopSeconds = 0.f;
		break;

	case EMLConveyorState::Stopped:
	default:
		ConveyorStopSeconds += Dt;
		break;
	}
}

void FMicrologicSimCore::SyncConveyorInterlockWire()
{
	if (!Config.Sim.bAutoConveyorInterlockWire)
	{
		return;
	}
	const int32 Channel = FindChannelByType(EMLInputType::Conveyor);
	if (Channel != INDEX_NONE)
	{
		SetInputRaw(Channel, IsConveyorRunning());
	}
}

bool FMicrologicSimCore::CanEngageWash() const
{
	// The Conveyor input tells the controller the conveyor is on.
	// Without this input, the controller will not engage a wash.
	const int32 Channel = FindChannelByType(EMLInputType::Conveyor);
	return Channel != INDEX_NONE && GetInputCommitted(Channel);
}

// ---------------------------------------------------------------------------
// Anti-collision
// ---------------------------------------------------------------------------

void FMicrologicSimCore::UpdateAntiCollision(float Dt)
{
	const bool bACInput = HasCommittedInput(EMLInputType::AntiCollision);

	// Pad cleared during the slow-down countdown: threat gone, resume running.
	if (ConveyorState == EMLConveyorState::SlowingDown && !bACInput)
	{
		SlowDownRemaining = 0.f;
		SetConveyorState(EMLConveyorState::Running);
	}

	// Restart after the lead vehicle clears the pad.
	if (bAntiCollisionHold && !bACInput)
	{
		bAntiCollisionHold = false;
		bACTripped = false;
		if (Config.AntiCollision.AfterClearsActivateService != 0)
		{
			ExecuteService(Config.AntiCollision.AfterClearsActivateService);
		}
		return;
	}

	if (!bACInput)
	{
		bACTripped = false;
	}

	const FMLRelayConfig* ACRelay = (Config.AntiCollision.RelayNumber != 0)
		? FindRelayConfig(Config.AntiCollision.RelayNumber) : nullptr;
	if (!ACRelay || !bACInput || !IsConveyorRunning())
	{
		return;
	}

	// Slow-down countdown already in progress.
	if (ConveyorState == EMLConveyorState::SlowingDown)
	{
		SlowDownRemaining -= Dt;
		if (SlowDownRemaining <= 0.f)
		{
			RequestConveyorStop(EMLStopReason::AntiCollision);
		}
		return;
	}

	if (bACTripped)
	{
		return;
	}

	// A trailing car reaching the A/C relay's device position while the pad is
	// occupied means it is getting too close to the lead vehicle.
	bool bTrailingCarAtStopPoint = false;
	for (const FCar& Car : Cars_Internal)
	{
		if (EvaluateRelayWindow(*ACRelay, Car))
		{
			bTrailingCarAtStopPoint = true;
			break;
		}
	}
	if (!bTrailingCarAtStopPoint)
	{
		return;
	}

	bACTripped = true;

	if (Config.AntiCollision.SlowDownService != 0)
	{
		ExecuteService(Config.AntiCollision.SlowDownService);
	}
	if (Config.AntiCollision.SlowDownHornService != 0)
	{
		ExecuteService(Config.AntiCollision.SlowDownHornService);
	}

	if (Config.AntiCollision.SlowDownTimeSeconds > 0.f)
	{
		SlowDownRemaining = Config.AntiCollision.SlowDownTimeSeconds;
		SetConveyorState(EMLConveyorState::SlowingDown);
	}
	else
	{
		// Slow-down time 0: the conveyor shuts off immediately (shut-off takes precedence).
		RequestConveyorStop(EMLStopReason::AntiCollision);
	}
}

// ---------------------------------------------------------------------------
// Inactivity
// ---------------------------------------------------------------------------

void FMicrologicSimCore::UpdateInactivity(float Dt)
{
	if (!IsConveyorRunning() || Config.Conveyor.InactivityTimeoutSeconds <= 0.f ||
		Config.Conveyor.ShutOffService == 0)
	{
		InactivitySeconds = 0.f;
		return;
	}

	if (Cars_Internal.Num() > 0)
	{
		InactivitySeconds = 0.f;
		return;
	}

	InactivitySeconds += Dt;
	if (InactivitySeconds < Config.Conveyor.InactivityTimeoutSeconds)
	{
		return;
	}

	// Relays flagged with Inactivity Check must be off before auto shut-off.
	for (const FMLRelayConfig& Relay : Config.Relays)
	{
		if (Relay.bActive && Relay.bInactivityCheck && GetRelayLogical(Relay.RelayNumber))
		{
			return;
		}
	}

	InactivitySeconds = 0.f;
	ExecuteService(Config.Conveyor.ShutOffService);
	// ExecuteService handles the stop via the Shut Off button designation; make
	// sure the stop reason reflects inactivity for the dashboard.
	if (ConveyorState == EMLConveyorState::Stopped)
	{
		LastStopReason = EMLStopReason::Inactivity;
	}
}

// ---------------------------------------------------------------------------
// Momentary actions
// ---------------------------------------------------------------------------

void FMicrologicSimCore::UpdateMomentaries(float Dt)
{
	for (int32 Index = Momentaries.Num() - 1; Index >= 0; --Index)
	{
		FMomentaryAction& Action = Momentaries[Index];
		if (!Action.bStarted)
		{
			Action.DelayRemaining -= Dt;
			if (Action.DelayRemaining <= 0.f)
			{
				Action.bStarted = true;
			}
			else
			{
				continue;
			}
		}

		Action.DurationRemaining -= Dt;
		if (Action.DurationRemaining <= 0.f)
		{
			Momentaries.RemoveAt(Index);
		}
	}
}

// ---------------------------------------------------------------------------
// Relay evaluation
// ---------------------------------------------------------------------------

float FMicrologicSimCore::ThresholdFor(EMLTurnReference Ref, float OffsetFeet, float DevicePos)
{
	return DevicePos + SignedOffset(Ref, OffsetFeet);
}

float FMicrologicSimCore::ReferencePosition(EMLTurnReference Ref, const FCar& Car) const
{
	return RefUsesFront(Ref) ? Car.State.FrontPositionFeet : Car.State.RearPositionFeet();
}

bool FMicrologicSimCore::EvaluateRelayWindow(const FMLRelayConfig& RelayCfg, const FCar& Car) const
{
	const FMLFunctionConfig& Fn = RelayCfg.Function;
	if (Fn.Type == EMLFunctionType::None || Fn.Type == EMLFunctionType::Light)
	{
		return false;
	}

	const float D = Fn.DevicePositionFeet;
	const float Front = Car.State.FrontPositionFeet;
	const float Rear = Car.State.RearPositionFeet();
	const float Mid = Front - Car.State.LengthFeet * 0.5f;
	const float CabEnd = (Car.State.CabLengthFeet > 0.f) ? (Front - Car.State.CabLengthFeet) : Rear;

	const float OnThreshold = ThresholdFor(Fn.TurnOnReference, Fn.TurnOnFeet, D);
	const float OffThreshold = ThresholdFor(Fn.TurnOffReference, Fn.TurnOffFeet, D);

	bool bOn = false;

	switch (Fn.Type)
	{
	case EMLFunctionType::VehicleLength:
		// References fully config-driven: default on at front, off at rear.
		bOn = ReferencePosition(Fn.TurnOnReference, Car) >= OnThreshold &&
		      ReferencePosition(Fn.TurnOffReference, Car) < OffThreshold;
		break;

	case EMLFunctionType::FrontOfVehicle:
		bOn = Front >= OnThreshold && Front < OffThreshold;
		break;

	case EMLFunctionType::RearOfVehicle:
		bOn = Rear >= OnThreshold && Rear < OffThreshold;
		break;

	case EMLFunctionType::FrontHalf:
		bOn = Front >= OnThreshold && Mid < OffThreshold;
		break;

	case EMLFunctionType::RearHalf:
		bOn = Mid >= OnThreshold && Rear < OffThreshold;
		break;

	case EMLFunctionType::PickupBed:
		bOn = Front >= OnThreshold && CabEnd < OffThreshold;
		break;

	case EMLFunctionType::AllTires:
	case EMLFunctionType::FrontTires:
	case EMLFunctionType::RearTires:
	{
		const TArray<float>& Tires = Car.State.TireOffsetsFeet;
		if (Tires.Num() == 0)
		{
			return false;
		}
		// Front/Rear tires require both sets defined by the tire switch.
		if ((Fn.Type == EMLFunctionType::FrontTires || Fn.Type == EMLFunctionType::RearTires) && Tires.Num() < 2)
		{
			return false;
		}

		auto TireActive = [&](float OffsetBehindFront)
		{
			const float TirePos = Front - OffsetBehindFront;
			return TirePos >= OnThreshold && TirePos < OffThreshold;
		};

		if (Fn.Type == EMLFunctionType::FrontTires)
		{
			bOn = TireActive(Tires[0]);
		}
		else if (Fn.Type == EMLFunctionType::RearTires)
		{
			bOn = TireActive(Tires.Last());
		}
		else
		{
			for (const float Offset : Tires)
			{
				if (TireActive(Offset))
				{
					bOn = true;
					break;
				}
			}
		}
		break;
	}

	default:
		bOn = false;
		break;
	}

	if (!bOn)
	{
		return false;
	}

	return !IsSuppressedByModifiers(Fn, Car, OnThreshold);
}

bool FMicrologicSimCore::IsSuppressedByModifiers(const FMLFunctionConfig& Fn, const FCar& Car, float WindowStartFront) const
{
	const float D = Fn.DevicePositionFeet;
	const float Front = Car.State.FrontPositionFeet;
	const float Rear = Car.State.RearPositionFeet();
	// Distance traveled since the window opened (vehicle-relative progress).
	const float S = Front - WindowStartFront;

	for (const FMLModifierConfig& Mod : Fn.Modifiers)
	{
		switch (Mod.Type)
		{
		case EMLModifierType::Bump:
		case EMLModifierType::MirrorBump:
			// Engage for StartFeet, retract for LengthFeet, then re-engage.
			if (S >= Mod.StartFeet && S < Mod.StartFeet + Mod.LengthFeet)
			{
				return true;
			}
			break;

		case EMLModifierType::FrontOfCar:
			// Wait LengthFeet after the front of the vehicle before turning on
			// (grill retract).
			if (S < Mod.LengthFeet)
			{
				return true;
			}
			break;

		case EMLModifierType::RearOfCar:
			// Turn off for the last LengthFeet of the vehicle (hitch retract).
			if (Rear >= D - Mod.LengthFeet)
			{
				return true;
			}
			break;

		case EMLModifierType::FrontAndRearOnly:
			// Keep the first StartFeet and the last LengthFeet; skip the middle.
			if (S >= Mod.StartFeet && Rear < D - Mod.LengthFeet)
			{
				return true;
			}
			break;

		case EMLModifierType::OpenPickupBed:
			// Retract over the open bed: once the cab has passed the device.
			if (Car.State.bOpenPickupBed && Car.State.CabLengthFeet > 0.f &&
				(Front - Car.State.CabLengthFeet) >= D)
			{
				return true;
			}
			break;

		default:
			break;
		}
	}

	return false;
}

void FMicrologicSimCore::EvaluateRelays()
{
	const bool bRollerUp = RollerSequence.bActive && (RollerSequence.Phase == 0 || RollerSequence.Phase == 2);

	for (int32 RelayNumber = 1; RelayNumber <= Relays.Num(); ++RelayNumber)
	{
		FRelayRuntime& Runtime = Relays[RelayNumber - 1];
		const FMLRelayConfig* Cfg = FindRelayConfig(RelayNumber);

		bool bWindow = false;

		if (Cfg && Cfg->bActive)
		{
			switch (Cfg->Type)
			{
			case EMLRelayType::Conveyor:
				bWindow = IsConveyorRunning();
				break;

			case EMLRelayType::Horn:
				bWindow = (ConveyorState == EMLConveyorState::Horn);
				break;

			case EMLRelayType::Roller:
				bWindow = bRollerUp;
				break;

			case EMLRelayType::Normal:
			default:
				if (Cfg->Function.Type == EMLFunctionType::Light)
				{
					// Light functions mirror the roller function (wash lights).
					bWindow = bRollerUp;
				}
				else
				{
					for (const FCar& Car : Cars_Internal)
					{
						const bool bOverride = Car.Program.OverrideRelays.Contains(RelayNumber);
						bool bApplies = bOverride || Car.Program.ProgrammedRelays.Contains(RelayNumber);
						if (Car.Program.CascadeOnFromRelay != 0 && RelayNumber >= Car.Program.CascadeOnFromRelay)
						{
							bApplies = true;
						}
						if (Car.Program.CascadeOffFromRelay != 0 && RelayNumber >= Car.Program.CascadeOffFromRelay)
						{
							bApplies = false;
						}
						if (!bApplies)
						{
							continue;
						}

						if (bOverride)
						{
							// Override: force the function for the entire vehicle length.
							const float D = Cfg->Function.DevicePositionFeet;
							if (Car.State.FrontPositionFeet >= D && Car.State.RearPositionFeet() < D)
							{
								bWindow = true;
							}
						}
						else if (EvaluateRelayWindow(*Cfg, Car))
						{
							bWindow = true;
						}

						if (bWindow)
						{
							break;
						}
					}

					// Look Back: keep the function on if a following vehicle is
					// within the configured distance behind the device.
					if (!bWindow && Runtime.bWindowActive && Cfg->LookBackFeet > 0.f)
					{
						const float D = Cfg->Function.DevicePositionFeet;
						for (const FCar& Car : Cars_Internal)
						{
							if (Car.State.FrontPositionFeet < D &&
								(D - Car.State.FrontPositionFeet) <= Cfg->LookBackFeet)
							{
								bWindow = true;
								break;
							}
						}
					}
				}
				break;
			}

			// Interlock Start: conveyor must run this long before the function may engage.
			if (bWindow && Cfg->InterlockStartSeconds > 0.f &&
				Cfg->Type == EMLRelayType::Normal &&
				ConveyorRunSeconds < Cfg->InterlockStartSeconds)
			{
				bWindow = false;
			}

			// Interlock Stop: once the conveyor has been off this long, the function turns off.
			if (bWindow && Cfg->InterlockStopSeconds > 0.f &&
				Cfg->Type == EMLRelayType::Normal &&
				!IsConveyorRunning() && ConveyorStopSeconds >= Cfg->InterlockStopSeconds)
			{
				bWindow = false;
			}
		}

		Runtime.bWindowActive = bWindow;

		// Compose logical state: window OR toggled OR momentary-on, minus momentary-off.
		bool bLogical = bWindow || ToggledRelays.Contains(RelayNumber);
		for (const FMomentaryAction& Action : Momentaries)
		{
			if (Action.RelayNumber == RelayNumber && Action.bStarted)
			{
				bLogical = Action.bTurnOn ? true : false;
			}
		}

		Runtime.bLogicalOn = bLogical;
	}

	ApplyPhysicalRelayStates();
}

void FMicrologicSimCore::ApplyPhysicalRelayStates()
{
	for (int32 RelayNumber = 1; RelayNumber <= Relays.Num(); ++RelayNumber)
	{
		FRelayRuntime& Runtime = Relays[RelayNumber - 1];

		bool bPhysical;
		switch (Runtime.Override)
		{
		case EMLRelayOverride::ForcedOn:
			bPhysical = true;
			break;
		case EMLRelayOverride::ForcedOff:
			bPhysical = false;
			break;
		case EMLRelayOverride::Auto:
		default:
			bPhysical = Runtime.bLogicalOn;
			break;
		}

		if (bPhysical != Runtime.bPhysicalOn)
		{
			Runtime.bPhysicalOn = bPhysical;
			OnRelayChanged.Broadcast(RelayNumber, bPhysical);
		}
	}
}

// ---------------------------------------------------------------------------
// Manual overrides
// ---------------------------------------------------------------------------

void FMicrologicSimCore::SetRelayOverride(int32 RelayNumber, EMLRelayOverride Override)
{
	if (RelayNumber >= 1 && RelayNumber <= Relays.Num())
	{
		Relays[RelayNumber - 1].Override = Override;
		ApplyPhysicalRelayStates();
	}
}

EMLRelayOverride FMicrologicSimCore::GetRelayOverride(int32 RelayNumber) const
{
	return (RelayNumber >= 1 && RelayNumber <= Relays.Num())
		? Relays[RelayNumber - 1].Override
		: EMLRelayOverride::Auto;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool FMicrologicSimCore::GetRelayPhysical(int32 RelayNumber) const
{
	return (RelayNumber >= 1 && RelayNumber <= Relays.Num()) && Relays[RelayNumber - 1].bPhysicalOn;
}

bool FMicrologicSimCore::GetRelayLogical(int32 RelayNumber) const
{
	return (RelayNumber >= 1 && RelayNumber <= Relays.Num()) && Relays[RelayNumber - 1].bLogicalOn;
}

bool FMicrologicSimCore::GetInputCommitted(int32 Channel) const
{
	return (Channel >= 1 && Channel <= Inputs.Num()) && Inputs[Channel - 1].bCommitted;
}

bool FMicrologicSimCore::GetInputRaw(int32 Channel) const
{
	return (Channel >= 1 && Channel <= Inputs.Num()) && Inputs[Channel - 1].bRaw;
}

int32 FMicrologicSimCore::GetNumRelays() const
{
	return Relays.Num();
}

int32 FMicrologicSimCore::GetNumInputs() const
{
	return Inputs.Num();
}

TArray<FMLRelayRuntime> FMicrologicSimCore::GetRelayStates() const
{
	TArray<FMLRelayRuntime> Result;
	Result.Reserve(Relays.Num());
	for (int32 RelayNumber = 1; RelayNumber <= Relays.Num(); ++RelayNumber)
	{
		FMLRelayRuntime State;
		State.RelayNumber = RelayNumber;
		State.bLogicalOn = Relays[RelayNumber - 1].bLogicalOn;
		State.bPhysicalOn = Relays[RelayNumber - 1].bPhysicalOn;
		State.Override = Relays[RelayNumber - 1].Override;
		Result.Add(State);
	}
	return Result;
}

TArray<FMLInputRuntime> FMicrologicSimCore::GetInputStates() const
{
	TArray<FMLInputRuntime> Result;
	Result.Reserve(Inputs.Num());
	for (int32 Channel = 1; Channel <= Inputs.Num(); ++Channel)
	{
		FMLInputRuntime State;
		State.Channel = Channel;
		State.bRaw = Inputs[Channel - 1].bRaw;
		State.bCommitted = Inputs[Channel - 1].bCommitted;
		Result.Add(State);
	}
	return Result;
}

// ---------------------------------------------------------------------------
// Lookups
// ---------------------------------------------------------------------------

bool FMicrologicSimCore::HasCommittedInput(EMLInputType Type) const
{
	const int32 Channel = FindChannelByType(Type);
	return Channel != INDEX_NONE && GetInputCommitted(Channel);
}

int32 FMicrologicSimCore::FindChannelByType(EMLInputType Type) const
{
	for (const FMLInputConfig& Input : Config.Inputs)
	{
		if (Input.Type == Type && Input.Channel >= 1 && Input.Channel <= Inputs.Num())
		{
			return Input.Channel;
		}
	}
	return INDEX_NONE;
}

const FMLInputConfig* FMicrologicSimCore::FindInputConfig(int32 Channel) const
{
	for (const FMLInputConfig& Input : Config.Inputs)
	{
		if (Input.Channel == Channel)
		{
			return &Input;
		}
	}
	return nullptr;
}

const FMLRelayConfig* FMicrologicSimCore::FindRelayConfig(int32 RelayNumber) const
{
	for (const FMLRelayConfig& Relay : Config.Relays)
	{
		if (Relay.RelayNumber == RelayNumber)
		{
			return &Relay;
		}
	}
	return nullptr;
}

const FMLServiceConfig* FMicrologicSimCore::FindServiceConfig(int32 ServiceNumber) const
{
	for (const FMLServiceConfig& Service : Config.Services)
	{
		if (Service.ServiceNumber == ServiceNumber)
		{
			return &Service;
		}
	}
	return nullptr;
}

float FMicrologicSimCore::EffectiveDebounce(const FMLInputConfig& InputCfg) const
{
	return InputCfg.DebounceSeconds > 0.f
		? InputCfg.DebounceSeconds
		: Config.RollerDefaults.DefaultInputDebounceSeconds;
}
