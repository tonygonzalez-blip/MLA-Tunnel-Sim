// Copyright Micrologic Associates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MicrologicTypes.h"

/**
 * XML serialization for the controller configuration — the Backup/Restore tab
 * writes and reads these files, mirroring the real controller's XML backups.
 */
namespace MicrologicXml
{
	/** Serialize a configuration to an XML document string. */
	MLA_TUNNEL_SIM_API FString ConfigToXml(const FMLControllerConfig& Config);

	/** Parse an XML document string. Returns false (and leaves OutConfig default) on malformed input. */
	MLA_TUNNEL_SIM_API bool ConfigFromXml(const FString& Xml, FMLControllerConfig& OutConfig);
}
