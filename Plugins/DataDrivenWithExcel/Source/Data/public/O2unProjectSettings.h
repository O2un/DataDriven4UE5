#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "O2unProjectSettings.generated.h"

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "O2un Excel Tool Settings"))
class O2UNDATAHELPER_API UO2unProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UO2unProjectSettings();

#if WITH_EDITORONLY_DATA
    UPROPERTY(Config, EditAnywhere, Category = "Editor Only", meta = (RelativePath))
    FDirectoryPath RawExcelDirectory;
#endif
    UPROPERTY(Config, EditAnywhere, Category = "Runtime", meta = (ContentDir))
    FDirectoryPath BinaryDirectory;

    static const UO2unProjectSettings& Get() { return *GetDefault<UO2unProjectSettings>(); }

    virtual FName GetCategoryName() const override { return TEXT("Project"); }
    virtual FName GetSectionName() const override { return TEXT("O2unExcelTool"); }
#if WITH_EDITOR
    virtual FText GetSectionText() const override { return NSLOCTEXT("O2un", "O2unExcelToolSettingsName", "O2un Excel Tool"); }
#endif
};