// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "O2unSingleton.h"
#include "O2unDllHelper.h"

class FO2unExcelHelperModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	O2un::FSmartDLLHandle _excelHelperHandle;
};

namespace O2un
{
	class O2UNEXCELHELPER_API FExcelHelper : public Singleton<FExcelHelper>
	{
	public:
		static void Testt(FString dataRoot);
	};
}