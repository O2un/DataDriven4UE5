// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.

#include "O2unExcelHelper.h"
#include "O2unCore.h"
#include "ExcelHelper.h"
#include "O2unDllHelper.h"
#include "O2unProjectSettings.h"
#include "ExcelHelperSubsystem.h"

void FO2unExcelHelperModule::StartupModule()
{
	_excelHelperHandle = O2un::FPluginHelper::LoadDLL("ExcelHelper");
	SetLogHandler(&FO2unLogProvider);

	// 빈 경로·없는 폴더는 DLL에 넘기지 않는다 (빈 값이면 DLL이 드라이브 루트에 쓴다).
	// 생성 메뉴(UExcelHelperSubsystem::GenerateDataScript)가 실행 직전에 다시 확인하고 넘긴다
	const FString generatedSourceDirectory = UO2unProjectSettings::Get().GetGeneratedSourceDirectoryAbsolute();
	FString message;
	if (false == UExcelHelperSubsystem::CheckDirectorySetting(generatedSourceDirectory, UExcelHelperSubsystem::GeneratedSourceDirectoryLabel, message))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataDrivenTool] %s"), *message);
		return;
	}

	FString ProjectName = FApp::GetProjectName();
	FString ProjectAPI = ProjectName.ToUpper() + TEXT("_API");
	SetCodeGenerateInfo(TCHAR_TO_UTF8(*ProjectAPI), TCHAR_TO_UTF8(*generatedSourceDirectory));
}

void FO2unExcelHelperModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FO2unExcelHelperModule, O2unExcelHelper)