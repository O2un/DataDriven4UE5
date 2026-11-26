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
const FString UExcelHelperSubsystem::RawExcelDirectoryLabel = TEXT("원본 엑셀 폴더(Raw Excel Directory)");
const FString UExcelHelperSubsystem::GeneratedSourceDirectoryLabel = TEXT("생성 코드 폴더(Generated Source Directory)");

bool UExcelHelperSubsystem::CheckDirectorySetting(const FString& absolutePath, const FString& settingName, FString& outMessage)
{
	if (absolutePath.IsEmpty())
	{
		outMessage = FString::Printf(TEXT("%s 경로가 설정되지 않았습니다. 프로젝트 세팅 > O2un Excel Tool을 확인해주세요."), *settingName);
		return false;
	}
	if (false == FPaths::DirectoryExists(absolutePath))
	{
		outMessage = FString::Printf(TEXT("%s 폴더가 없습니다: %s\n프로젝트 세팅 > O2un Excel Tool을 확인해주세요."), *settingName, *absolutePath);
		return false;
	}
	return true;
}

bool UExcelHelperSubsystem::RefreshExcelFileList()
{
	FString message;
	if (false == CheckDirectorySetting(UO2unProjectSettings::Get().GetRawExcelDirectoryAbsolute(), RawExcelDirectoryLabel, message))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataDrivenTool] %s"), *message);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(message), _alertTitle);
		return false;
	}

	LoadAllExcelFileList();
	return true;
}

void UExcelHelperSubsystem::GenerateDataScript()
{
	const UO2unProjectSettings& settings = UO2unProjectSettings::Get();
	const FString rawExcelDirectory = settings.GetRawExcelDirectoryAbsolute();
	const FString generatedSourceDirectory = settings.GetGeneratedSourceDirectoryAbsolute();

	// 빈 경로를 DLL에 넘기면 드라이브 루트(예: C:/Private/...)에 쓰게 되므로 여기서 막는다
	FString message;
	if (false == CheckDirectorySetting(rawExcelDirectory, RawExcelDirectoryLabel, message)
		|| false == CheckDirectorySetting(generatedSourceDirectory, GeneratedSourceDirectoryLabel, message))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataDrivenTool] %s"), *message);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(message), _alertTitle);
		return;
	}

	// 모듈 시작 때 경로가 잘못돼 건너뛰었거나 이후 설정이 바뀐 경우를 위해 생성 직전에 다시 넘긴다
	const FString projectApi = FString(FApp::GetProjectName()).ToUpper() + TEXT("_API");
	SetCodeGenerateInfo(TCHAR_TO_UTF8(*projectApi), TCHAR_TO_UTF8(*generatedSourceDirectory));
	GenerateAllDataClass(TCHAR_TO_UTF8(*rawExcelDirectory));

	FText out, fail;
	FGameProjectGenerationModule& GPGenModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	GPGenModule.UpdateCodeProject(fail, out);
}

void UExcelHelperSubsystem::GenerateAllDataAsset()
{
	if (false == RefreshExcelFileList())
	{
		return;
	}

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

	// 알림(throbber)을 띄우기 전에 확인한다 (예전에는 알림을 띄운 뒤 반환해 대기 상태 알림이 남았다)
	FString assetFolder = UO2unProjectSettings::Get().DataAssetDirectory.Path;
	if (assetFolder.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("데이터 에셋 경로가 설정되지 않았습니다 프로젝트 세팅을 확인해주세요")), _alertTitle);
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

	TArray<FString> missingSheets;
	for (UClass* dataClass : derivedClasses)
	{
		FString className = dataClass->GetName();
		if (dataClass->HasAnyClassFlags(CLASS_Abstract) || className.StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		// 시트를 엑셀 목록에서 못 찾으면(시트 이름 변경·삭제, 원본 엑셀 폴더 설정 오류) 에셋을 만들거나 저장하지 않는다
		FString sheetName = className.Replace(TEXT("DataAsset"), TEXT(""));
		const FString* sheetPath = _excelPathList.Find(sheetName);
		if (nullptr == sheetPath)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DataDrivenTool] %s: '%s' 시트를 원본 엑셀 폴더에서 찾을 수 없어 건너뜁니다."), *className, *sheetName);
			missingSheets.Add(sheetName);
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
			const FString& path = *sheetPath;
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
		if (missingSheets.IsEmpty())
		{
			NotificationItem->SetText(FText::FromString(TEXT("모든 에셋 저장 완료!")));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		}
		else
		{
			NotificationItem->SetText(FText::FromString(FString::Printf(TEXT("에셋 저장 완료 (시트를 찾지 못해 건너뜀 %d건)"), missingSheets.Num())));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		}
		NotificationItem->ExpireAndFadeout();
	}

	if (false == missingSheets.IsEmpty())
	{
		const FString message = FString::Printf(TEXT("원본 엑셀 폴더에서 다음 시트를 찾을 수 없어 건너뛰었습니다:\n%s\n\n시트 이름이나 원본 엑셀 폴더 설정을 확인해주세요."), *FString::Join(missingSheets, TEXT(", ")));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(message), _alertTitle);
	}
}

void UExcelHelperSubsystem::LoadAllExcelFileList()
{
	// 이전 호출 결과(삭제·이름이 바뀐 시트, 바뀌기 전 폴더)가 남지 않게 매번 새로 채운다
	_excelPathList.Reset();

	const char* result = GetExcelList(TCHAR_TO_UTF8(*UO2unProjectSettings::Get().GetRawExcelDirectoryAbsolute()));
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
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataDrivenTool] 엑셀 목록을 읽지 못했습니다 (GetExcelList 결과가 올바른 JSON이 아님)."));
	}
}
