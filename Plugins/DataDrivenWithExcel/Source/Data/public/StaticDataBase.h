// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once
#include "StaticDataKey.h"
#include "StaticDataBase.generated.h"

USTRUCT()
struct FStaticDataInitStruct
{
	GENERATED_BODY()

	UPROPERTY()
	FStaticDataKey _key;
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