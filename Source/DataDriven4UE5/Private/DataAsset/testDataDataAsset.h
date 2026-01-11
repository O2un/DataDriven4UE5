// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StaticDataAssetBase.h"
#include "testDataDataAsset.generated.h"

/**
 * Auto-generated Static Data Asset
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
class UtestDataStaticData;

UCLASS()
class DATADRIVEN4UE5_API UtestDataDataAsset : public UStaticDataAssetBase
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "StaticData")
    UtestDataStaticData* GetData(FStaticDataKey id, bool& success);
};
