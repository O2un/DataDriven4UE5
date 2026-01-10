#include "O2unExcelHelper.h"
#include "O2unCore.h"
#include "ExcelHelper.h"
#include "O2unDllHelper.h"
#include "O2unProjectSettings.h"
#include "GameProjectGenerationModule.h"

void FO2unExcelHelperModule::StartupModule()
{
	_excelHelperHandle = O2un::FPluginHelper::LoadDLL("ExcelHelper");
	SetLogHandler(&FO2unLogProvider);

	FString ProjectName = FApp::GetProjectName();
	FString ProjectAPI = ProjectName.ToUpper() + TEXT("_API");
	SetCodeGenerateInfo(TCHAR_TO_UTF8(*ProjectAPI), TCHAR_TO_UTF8(*UO2unProjectSettings::Get().GeneratedSourceDirectory.Path));
}

void FO2unExcelHelperModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FO2unExcelHelperModule, O2unExcelHelper)

namespace O2un
{
	void FExcelHelper::Testt(FString dataRoot)
	{
		GenerateAllDataClass(TCHAR_TO_UTF8(*dataRoot));

		FText out, fail;
		FGameProjectGenerationModule& GPGenModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
		GPGenModule.UpdateCodeProject(fail, out);
	}
}
