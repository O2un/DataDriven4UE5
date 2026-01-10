#pragma once

#include "CoreMinimal.h"
#include "StaticDataBase.h"
#include "testDataStaticData.generated.h"

/**
 * Auto-generated Static Data Class
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class DATADRIVEN4UE5_API UtestDataStaticData : public UStaticDataBase
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StaticData")
    int32 data;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StaticData")
    float value;


};