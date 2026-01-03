#include "O2unDllHelper.h"
#include "HAL/PlatformProcess.h"
#include <Interfaces/IPluginManager.h>
#include "Misc/Paths.h"

namespace O2un
{
    void FDllDeleter::operator()(void* handle) const
    {
        if (handle) {
            FPlatformProcess::FreeDllHandle(handle);
            UE_LOG(LogTemp, Log, TEXT("[Core] DLL Released safely."));
        }
    }

	FSmartDLLHandle FPluginHelper::LoadDLL(const FString& dllName)
	{
        auto path = GetDllPath(dllName);
        if (FPaths::FileExists(*path))
        {
            void* raw = FPlatformProcess::GetDllHandle(*path);
            return raw ? FSmartDLLHandle(raw) : FSmartDLLHandle(nullptr);
        }

        return nullptr;
	}

    const FString FPluginHelper::puglinName = "DataDrivenWithExcel";
    TSharedPtr<IPlugin> FPluginHelper::GetPlugin()
    {
        return IPluginManager::Get().FindPlugin(TEXT("DataDrivenWithExcel"));
    }

    FString FPluginHelper::GetDllPath(const FString& dllName)
    {
        //ExcelHelper
        FString baseDir = IPluginManager::Get().FindPlugin(TEXT("DataDrivenWithExcel"))->GetBaseDir();
        return FPaths::Combine(baseDir, TEXT("Binaries"), TEXT("Win64"), *dllName ,TEXT(".dll"));
    }
}

