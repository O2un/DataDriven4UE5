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

private:
	TMap<FString, FString> _excelPathList;
	static const FText _alertTitle;
};
