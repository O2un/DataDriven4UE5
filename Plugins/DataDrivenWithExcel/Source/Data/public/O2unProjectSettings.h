// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
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
    /** 원본 엑셀 폴더. 프로젝트 폴더 기준 상대 경로(예: RawData) 또는 절대 경로 */
    UPROPERTY(Config, EditAnywhere, Category = "Editor Only")
    FDirectoryPath RawExcelDirectory;

    /** 생성 코드를 쓸 게임 모듈 폴더. 프로젝트 폴더 기준 상대 경로(예: Source/DataDriven4UE5) 또는 절대 경로 */
    UPROPERTY(Config, EditAnywhere, Category = "Editor Only", meta = (ConfigRestartRequired = false))
    FDirectoryPath GeneratedSourceDirectory;

    /** 상대 경로는 프로젝트 폴더 기준으로 풀어 절대 경로로 돌려준다. 빈 값은 빈 값 */
    static FString ResolveProjectRelativeDirectory(const FString& path);
    FString GetRawExcelDirectoryAbsolute() const { return ResolveProjectRelativeDirectory(RawExcelDirectory.Path); }
    FString GetGeneratedSourceDirectoryAbsolute() const { return ResolveProjectRelativeDirectory(GeneratedSourceDirectory.Path); }

    /** 프로젝트 폴더 아래의 절대 경로면 프로젝트 기준 상대 경로로 바꾼다. 그 밖의 경로·상대 경로·빈 값은 그대로 */
    static FString MakeProjectRelativeIfInside(const FString& path);
#endif
    UPROPERTY(Config, EditAnywhere, Category = "Runtime", meta = (ContentDir))
    FDirectoryPath DataAssetDirectory;

    static const UO2unProjectSettings& Get() { return *GetDefault<UO2unProjectSettings>(); }

    virtual FName GetCategoryName() const override { return TEXT("Project"); }
    virtual FName GetSectionName() const override { return TEXT("O2unExcelTool"); }
#if WITH_EDITOR
    virtual FText GetSectionText() const override { return NSLOCTEXT("O2un", "O2unExcelToolSettingsName", "O2un Excel Tool"); }

    /** 폴더 선택 버튼이 넣은 절대 경로를 프로젝트 기준 상대 경로로 바꿔 저장한다 (DefaultGame.ini에 PC별 절대 경로가 남지 않게) */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif
};