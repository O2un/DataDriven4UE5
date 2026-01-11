// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.

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
