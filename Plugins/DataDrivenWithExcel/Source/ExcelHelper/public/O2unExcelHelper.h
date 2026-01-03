// Copyright Epic Games, Inc. All Rights Reserved.

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
		static void Test();
	};
}