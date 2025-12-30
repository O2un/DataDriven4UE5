// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UToolMenu;
class SWindow;

class FO2unEditorToolModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OnMenuClicked();
	void FillMenu(UToolMenu* InMenu);

	TWeakPtr<SWindow> _toolWindow;
};
