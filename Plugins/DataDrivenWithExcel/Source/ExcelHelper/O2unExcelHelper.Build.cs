// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class O2unExcelHelper : ModuleRules
{
    public O2unExcelHelper(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
                Path.Combine(ModuleDirectory, "public")
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
                Path.Combine(ModuleDirectory, "private")
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
				// ... add other public dependencies that you statically link with here ...
                "O2unCore",
                "O2unDataHelper",
                "GameProjectGeneration",
                "AssetTools",
                "AssetRegistry",
                "UnrealEd",
                "EditorSubsystem",
                "Json",
                "JsonUtilities",
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
				// ... add private dependencies that you statically link with here ...
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

        if (Target.Type == TargetType.Editor)
        {
            string dllPath = Path.Combine(ModuleDirectory, "./");

            PublicIncludePaths.Add(Path.Combine(dllPath, "Include"));
            PublicAdditionalLibraries.Add(Path.Combine(dllPath,"ExcelHelper.lib"));
            //PublicDelayLoadDLLs.Add("ExcelHelper.dll");
            RuntimeDependencies.Add("$(BinaryOutputDir)/ExcelHelper.dll", Path.Combine(dllPath, "ExcelHelper.dll"));
        }
    }
}
