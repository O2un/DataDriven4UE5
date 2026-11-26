// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once

#include <stdint.h>

#ifdef EXCELHELPER_EXPORTS
#define O2UN_API __declspec(dllexport)
#else
#define O2UN_API __declspec(dllimport)
#endif // EXCELHELPER_EXPORTS

// 로그 등급 (logCallback의 level 값)
#define O2UN_LOG_INFO    0
#define O2UN_LOG_WARNING 1
#define O2UN_LOG_ERROR   2
#define O2UN_LOG_FATAL   3

extern "C"
{
	// message는 UTF-8, 콜백 안에서만 유효하다
	typedef void(*logCallback)(int32_t level, const char* message);
	// 등급 추가로 시그니처가 바뀌어 이름을 바꿨다 (예전 DLL과 섞이면 로드 시점에 실패하도록)
	O2UN_API void SetLogHandlerWithLevel(logCallback c);
	O2UN_API void SetCodeGenerateInfo(const char* projectName, const char* root);
	O2UN_API void GenerateAllDataClass(const char* root);
	O2UN_API const char* LoadExcel(const char* path, const char* className);
	O2UN_API const char* GetExcelList(const char* root);
}
