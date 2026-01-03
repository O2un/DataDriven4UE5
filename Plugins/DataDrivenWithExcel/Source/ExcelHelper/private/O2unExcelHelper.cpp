#include "O2unExcelHelper.h"
#include "ExcelHelper.h"
#include "O2unDllHelper.h"

void FO2unExcelHelperModule::StartupModule()
{
	_excelHelperHandle = O2un::FPluginHelper::LoadDLL("ExcelHelper");
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
	void FExcelHelper::Test()
	{
		PrintDLL();
	}
}
