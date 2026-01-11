#pragma once
// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "CoreMinimal.h"
#include "StaticDataKey.generated.h"

USTRUCT(BlueprintType)
struct FStaticDataKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StaticData")
	int32 _group = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StaticData")
	int32 _id = 0;

	FStaticDataKey() : _group(0), _id(0)
	{
	}

	FStaticDataKey(int32 group, int32 id) : _group(group), _id(id)
	{
	}

	bool operator==(const FStaticDataKey& other) const { return _id == other._id && _group == other._group; }
	friend uint32 GetTypeHash(const FStaticDataKey& key) { return HashCombine(GetTypeHash(key._id), GetTypeHash(key._group)); }
	bool isDefined() { return 0 != _id || 0 != _group; }
	int64 Key() const { return (static_cast<int64>(_group) << 32) | (static_cast<int64>(_id));}
};

