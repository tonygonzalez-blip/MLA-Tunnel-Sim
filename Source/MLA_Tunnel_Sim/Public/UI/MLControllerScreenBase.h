// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLControllerScreenBase.generated.h"

class UMicrologicControllerSubsystem;

/**
 * Shared base for every Micrologic controller UI screen.
 *
 * Provides access to the controller subsystem and a RefreshFromController()
 * hook that runs once from NativeConstruct; screens override it to (re)load
 * their state. All subwidget bindings in derived classes are
 * BindWidgetOptional, so every override must null-check its widgets —
 * partial designer layouts must still run.
 */
UCLASS(Abstract)
class MLA_TUNNEL_SIM_API UMLControllerScreenBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** The tunnel-controller subsystem (may be null very early in shutdown/teardown). */
	UFUNCTION(BlueprintPure, Category = "Micrologic|UI")
	UMicrologicControllerSubsystem* GetController() const;

	/** Reload widget state from the controller. Called from NativeConstruct. */
	virtual void RefreshFromController() {}
};
