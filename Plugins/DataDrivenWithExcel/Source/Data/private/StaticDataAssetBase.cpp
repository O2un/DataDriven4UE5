// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.

#include "StaticDataAssetBase.h"
#include <JsonObjectConverter.h>

#if WITH_EDITOR
#include "SStaticDataVisualizer.h"
#endif

void UStaticDataAssetBase::Load(const char* json, FString name)
{
	FString jsonString = UTF8_TO_TCHAR(json);

	FString className = FString::Printf(TEXT("%sStaticData"), *name);
	UClass* dataClass = FindFirstObject<UClass>(*className);

	FString structName = FString::Printf(TEXT("%sInitStruct"), *name);
	UScriptStruct* initStruct = FindFirstObject<UScriptStruct>(*structName);

	if (!dataClass)
	{
		dataClass = FindFirstObject<UClass>(*FString::Printf(TEXT("U%s"), *className));
	}
	if (!initStruct)
	{
		initStruct = FindFirstObject<UScriptStruct>(*FString::Printf(TEXT("F%s"), *structName));
	}

	if (!dataClass || !initStruct)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] 로드 실패: Class(%s) 또는 Struct(%s)를 찾을 수 없습니다."),
			*name, *className, *structName);
		return;
	}

	TArray<TSharedPtr<FJsonValue>> jsonArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonString);
	if (FJsonSerializer::Deserialize(reader, jsonArray))
	{
		_dataList.Empty();
		for (const auto& value : jsonArray)
		{
			TSharedPtr<FJsonObject> jsonObj = value->AsObject();
			if (!jsonObj.IsValid()) continue;

			void* structPtr = FMemory::Malloc(initStruct->GetStructureSize());
			initStruct->InitializeStruct(structPtr);

			if (FJsonObjectConverter::JsonObjectToUStruct(jsonObj.ToSharedRef(), initStruct, structPtr))
			{
				UStaticDataBase* newData = NewObject<UStaticDataBase>(this, dataClass);
				if (newData)
				{
					newData->Initialize(*static_cast<FStaticDataInitStruct*>(structPtr));

					FStaticDataKey key = static_cast<FStaticDataInitStruct*>(structPtr)->_key;
					_dataList.Add(key, newData);
				}
			}

			initStruct->DestroyStruct(structPtr);
			FMemory::Free(structPtr);
		}
	}
}

#if WITH_EDITOR
void UStaticDataAssetBase::OpenVisualizer()
{
	TSharedRef<SWindow> VisualizerWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Data Visualizer")))
		.ClientSize(FVector2D(800, 600))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		[
			SNew(SStaticDataVisualizer)
				.TargetAsset(this)
		];

	FSlateApplication::Get().AddWindow(VisualizerWindow);
}

void UStaticDataAssetBase::EnumerateData(TFunctionRef<void(const UStaticDataBase*)> Visitor) const
{
	for (const auto& Pair : _dataList)
	{
		if (Pair.Value)
		{
			Visitor(Pair.Value);
		}
	}
}

UClass* UStaticDataAssetBase::GetDataClass() const
{
	if (_dataList.Num() > 0)
	{
		return _dataList.CreateConstIterator().Value()->GetClass();
	}
	return nullptr;
}
#endif