// Fill out your copyright notice in the Description page of Project Settings.


#include "ExcelHelperSubsystem.h"
#include "ExcelHelper.h"
#include "O2unProjectSettings.h"
#include "GameProjectGenerationModule.h"
#include "StaticDataAssetBase.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "StaticDataSubsystem.h"
#include "Misc/MessageDialog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

const FText UExcelHelperSubsystem::_alertTitle = FText::FromString(TEXT("알림"));
void UExcelHelperSubsystem::GenerateDataScript()
{
	GenerateAllDataClass(TCHAR_TO_UTF8(*UO2unProjectSettings::Get().RawExcelDirectory.Path));

	FText out, fail;
	FGameProjectGenerationModule& GPGenModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	GPGenModule.UpdateCodeProject(fail, out);
}

void UExcelHelperSubsystem::GenerateAllDataAsset()
{
	LoadAllExcelFileList();

	
	if (!GEngine)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("엔진이 로드 되지 않았습니다.")), _alertTitle);

		return;
	}

	auto* subSystem = GEngine->GetEngineSubsystem<UStaticDataSubsystem>();
	if(!subSystem)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("서브시스템이 아직 로드 되지 않았습니다.")), _alertTitle);

		return;
	}

	FNotificationInfo Info(FText::FromString(TEXT("데이터 처리하는 중...")));
	Info.bUseThrobber = true;
	Info.FadeInDuration = 0.1f;
	Info.ExpireDuration = 5.0f;

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	TArray<UClass*> derivedClasses;
	GetDerivedClasses(UStaticDataAssetBase::StaticClass(), derivedClasses, true);

	IAssetTools& tool = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FAssetRegistryModule& module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FString assetFolder = UO2unProjectSettings::Get().DataAssetDirectory.Path;
	if (assetFolder.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("데이터 에셋 경로가 설정되지 않았습니다 프로젝트 세팅을 확인해주세요")), _alertTitle);
		return;
	}

	for (UClass* dataClass : derivedClasses)
	{
		FString className = dataClass->GetName();
		if (dataClass->HasAnyClassFlags(CLASS_Abstract) || className.StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		FString assetPath = assetFolder / className;
		UObject* assetObject = nullptr;
		UStaticDataAssetBase* targetAsset = nullptr;

		targetAsset = LoadObject<UStaticDataAssetBase>(nullptr, *assetPath);
		if (!targetAsset)
		{
			targetAsset = Cast<UStaticDataAssetBase>(tool.CreateAsset(className, assetFolder, dataClass, nullptr));
			if (targetAsset)
			{
				FAssetRegistryModule::AssetCreated(targetAsset);
				targetAsset->MarkPackageDirty();
			}
		}

		if (targetAsset)
		{
			FString sheetName = className.Replace(TEXT("DataAsset"), TEXT(""));
			auto path = *_excelPathList.Find(sheetName);
			if (false == path.IsEmpty())
			{
				const char* result = LoadExcel(TCHAR_TO_UTF8(*path), TCHAR_TO_UTF8(*sheetName));
				targetAsset->Load(result, sheetName);
			}

			TArray<UPackage*> PackagesToSave;
			PackagesToSave.Add(targetAsset->GetOutermost());
			FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);

			subSystem->RegisterAsset(dataClass, targetAsset);
		}
	}

	if (NotificationItem.IsValid())
	{
		NotificationItem->SetText(FText::FromString(TEXT("모든 에셋 저장 완료!")));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		NotificationItem->ExpireAndFadeout();
	}
}

void UExcelHelperSubsystem::LoadAllExcelFileList()
{
	const char* result = GetExcelList(TCHAR_TO_UTF8(*UO2unProjectSettings::Get().RawExcelDirectory.Path));
	FString jsonString = UTF8_TO_TCHAR(result);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(jsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		for (auto& pair : JsonObject->Values)
		{
			_excelPathList.Emplace(pair.Key, pair.Value->AsString());
		}
	}
}
