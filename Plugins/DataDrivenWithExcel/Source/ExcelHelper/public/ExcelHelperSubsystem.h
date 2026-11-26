// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "ExcelHelperSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class O2UNEXCELHELPER_API UExcelHelperSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
	
public:
	void GenerateDataScript();
	void GenerateAllDataAsset();
	void LoadAllExcelFileList();

	/** 설정 폴더(절대 경로)가 비었거나 없으면 false + 한국어 사유. settingName은 메시지에 쓸 설정 이름 */
	static bool CheckDirectorySetting(const FString& absolutePath, const FString& settingName, FString& outMessage);

	static const FString RawExcelDirectoryLabel;
	static const FString GeneratedSourceDirectoryLabel;

private:
	/** 원본 엑셀 폴더를 확인하고 시트 -> 엑셀 경로 목록을 새로 읽는다. 실패하면 알림을 띄우고 false */
	bool RefreshExcelFileList();

	TMap<FString, FString> _excelPathList;
	static const FText _alertTitle;
};
