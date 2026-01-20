// Fill out your copyright notice in the Description page of Project Settings.


#include "StaticDataSubsystem.h"

void UStaticDataSubsystem::RegisterAsset(UClass* classType, UStaticDataAssetBase* dataAsset)
{
	if (classType && dataAsset)
	{
		_dataAssets.Emplace(classType, dataAsset);
	}
}

template<typename T>
inline T* UStaticDataSubsystem::GetAsset() const
{
	UClass* type = T::StaticClass();
	UStaticDataAssetBase* const* assetPtr = _dataAssets.Find(type);
	return (assetPtr) ? Cast<T>(*assetPtr) : nullptr;
}

template<typename T>
inline auto UStaticDataSubsystem::GetData(const FStaticDataKey& key) const -> decltype(std::declval<T>().GetData(key))
{
	T* dataAsset = GetAsset<T>();
	return (dataAsset) ? dataAsset->GetData(key) : nullptr;
}