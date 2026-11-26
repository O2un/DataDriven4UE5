// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
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

// ExcelHelper DLL 로그 콜백. level 값은 ExcelHelper.h의 O2UN_LOG_* (0 Info, 1 Warning, 2 Error, 3 Fatal)
// DLL의 Fatal은 "작업 중단" 의미라 에디터를 종료시키는 UE Fatal 대신 Error로 남긴다
inline void FO2unLogProvider(int32 level, const char* message)
{
	const FString text = UTF8_TO_TCHAR(message);
	switch (level)
	{
	case 0:
		UE_LOG(LogTemp, Display, TEXT("[DataDrivenTool] %s"), *text);
		break;
	case 1:
		UE_LOG(LogTemp, Warning, TEXT("[DataDrivenTool] %s"), *text);
		break;
	case 3:
		UE_LOG(LogTemp, Error, TEXT("[DataDrivenTool][치명] %s"), *text);
		break;
	case 2:
	default:
		UE_LOG(LogTemp, Error, TEXT("[DataDrivenTool] %s"), *text);
		break;
	}
}
