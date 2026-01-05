#pragma once
// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FO2unCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};

void FO2unLogProvider(const char* message)
{
	UE_LOG(LogTemp, Display, TEXT("[DataDrivenTool] %s"), UTF8_TO_TCHAR(message));
}