// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StaticDataBase.h"
#include "UBaseStaticDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class O2UNDATAHELPER_API UUBaseStaticDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Instanced, Category = "StaticData", meta = (ShowOnlyInnerProperties))
	TMap<FStaticDataKey, UStaticDataBase*> _dataList;
};
