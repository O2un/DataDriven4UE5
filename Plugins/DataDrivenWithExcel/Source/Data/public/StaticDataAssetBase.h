// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StaticDataBase.h"
#include "StaticDataAssetBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType)
class O2UNDATAHELPER_API UStaticDataAssetBase : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Instanced, Category = "StaticData", meta = (ShowOnlyInnerProperties))
	TMap<FStaticDataKey, UStaticDataBase*> _dataList;
};
