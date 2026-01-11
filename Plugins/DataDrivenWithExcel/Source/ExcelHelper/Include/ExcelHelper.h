// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once
#include "pch.h"

extern "C" 
{
	typedef void(*logCallback)(const char*);
	O2UN_API void SetLogHandler(logCallback c);
	O2UN_API void SetCodeGenerateInfo(const char* projectName, const char* root);
	O2UN_API void GenerateAllDataClass(const char* root);
}


