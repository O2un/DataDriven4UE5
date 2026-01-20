// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "StaticDataAssetBase.h"
#include "StaticDataKey.h"
#include "StaticDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class O2UNDATAHELPER_API UStaticDataSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
public:
	void RegisterAsset(UClass* classType, UStaticDataAssetBase* dataAsset);

	template<typename T>
	T* GetAsset() const;

	template<typename T>
	auto GetData(const FStaticDataKey& key) const -> decltype(std::declval<T>().GetData(key));
	
private:
	UPROPERTY()
	TMap<UClass*, UStaticDataAssetBase*> _dataAssets;
};