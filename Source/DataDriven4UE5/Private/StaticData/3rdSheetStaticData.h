// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "StaticDataBase.h"
#include "3rdSheetStaticData.generated.h"

/**
 * Auto-generated Static Data Class
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
USTRUCT()
struct F3rdSheetInitStruct : public FStaticDataInitStruct
{
GENERATED_BODY()

UPROPERTY()
	FString _data;

};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class DATADRIVEN4UE5_API U3rdSheetStaticData : public UStaticDataBase
{
    GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StaticData")
	FString _data;


    virtual void Initialize(const FStaticDataInitStruct& data) override;
};