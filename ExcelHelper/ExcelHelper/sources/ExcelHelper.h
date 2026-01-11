// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once

#ifdef EXCELHELPER_EXPORTS
#define O2UN_API __declspec(dllexport)
#else
#define O2UN_API __declspec(dllimport)
#endif // EXCELHELPER_EXPORTS

extern "C" 
{
	typedef void(*logCallback)(const char*);
	O2UN_API void SetLogHandler(logCallback c);
	O2UN_API void SetCodeGenerateInfo(const char* projectName, const char* root);
	O2UN_API void GenerateAllDataClass(const char* root);
}


