// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "StaticDataBase.h"

void UStaticDataBase::Initialize(const FStaticDataInitStruct& data)
{
	if (_initialized)
	{
		ensureMsgf(false, TEXT("StaticData already Initialized"));
		return;
	}

	_key = data._key;
	_initialized = true;
}
