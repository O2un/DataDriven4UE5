#pragma once
#include "StaticDataKey.h"
#include "StaticDataBase.generated.h"

struct FStaticDataInitStruct
{
	FStaticDataKey _key;
	virtual ~FStaticDataInitStruct() = default;
};

UCLASS(Abstract, BlueprintType)
class O2UNDATAHELPER_API UStaticDataBase : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="StaticData")
	FStaticDataKey Key() const { return _key; }

	virtual void Initialize(const FStaticDataInitStruct& data);

private:
	bool _initialized = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StaticData", meta = (DisplayName = "Unique Data Key", AllowPrivateAccess = "true"))
	FStaticDataKey _key;
};