// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "ExcelHelper.h"
import <exception>;
import <format>;
import <string>;
import IExcelLoader;
import ILogger;
import IStringHelper;

namespace
{
	// 예외가 extern "C" 경계를 넘지 않게 한다. 실패 시 오류 로그를 남기고 fallback을 돌려준다
	template <typename Func, typename Result>
	Result GuardCBoundary(const char* functionName, Result fallback, Func&& func)
	{
		try
		{
			return func();
		}
		catch (const std::exception& e)
		{
			Logger::Error(std::format("{} 처리 중 예외: {}", functionName, CommonHelper::ToUtf8FromSystem(e.what())));
		}
		catch (...)
		{
			Logger::Error(std::format("{} 처리 중 알 수 없는 예외", functionName));
		}
		return fallback;
	}
}

void SetLogHandlerWithLevel(logCallback c)
{
	Logger::Instance().SetLogHandler(c);
}

void SetCodeGenerateInfo(const char* projectName, const char* path)
{
	GuardCBoundary("SetCodeGenerateInfo", 0, [&] { ExcelLoader::Instance().SetCodeGeneratePath(projectName, path); return 0; });
}

void GenerateAllDataClass(const char* root)
{
	GuardCBoundary("GenerateAllDataClass", 0, [&] { ExcelLoader::Instance().GenerateAllDataClass(root); return 0; });
}

// 반환 문자열은 DLL 내부 버퍼 소유 — 다음 LoadExcel/GetExcelList 호출 전까지만 유효 (기존 동작)
const char* LoadExcel(const char* path, const char* className)
{
	return GuardCBoundary("LoadExcel", "", [&] { return ExcelLoader::Instance().LoadExcel(path, className); });
}

const char* GetExcelList(const char* root)
{
	return GuardCBoundary("GetExcelList", "", [&] { return ExcelLoader::Instance().GetExcelList(root); });
}
