// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "ExcelHelper.h"
#include <cstdlib>
#include <cstring>
import <exception>;
import <format>;
import <string>;
import <string_view>;
import IDataApi;
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

	constexpr int32_t RcInvalidArgument = -1;
	constexpr int32_t RcInternal = -99;

	// O2un_FreeString과 짝: malloc/free
	char* AllocateString(std::string_view text) noexcept
	{
		char* buffer = static_cast<char*>(std::malloc(text.size() + 1));
		if (nullptr != buffer)
		{
			std::memcpy(buffer, text.data(), text.size());
			buffer[text.size()] = '\0';
		}
		return buffer;
	}

	// O2un_* 공통: outJson 확인, 예외 -> -99, 결과 JSON 할당. 예외는 C 경계를 넘지 않는다
	template <typename Func>
	int32_t RunDataApi(const char* functionName, char** outJson, Func&& func) noexcept
	{
		if (nullptr == outJson)
		{
			return RcInvalidArgument;
		}
		*outJson = nullptr;

		int32_t code = RcInternal;
		std::string json;
		try
		{
			try
			{
				code = func(json);
			}
			catch (const std::exception& e)
			{
				const std::string what = CommonHelper::ToUtf8FromSystem(e.what());
				Logger::Error(std::format("{} 처리 중 예외: {}", functionName, what));
				code = RcInternal;
				json = DataApi::MakeErrorJson("internal", std::format("internal error: {}", what));
			}
			catch (...)
			{
				Logger::Error(std::format("{} 처리 중 알 수 없는 예외", functionName));
				code = RcInternal;
				json = DataApi::MakeErrorJson("internal", "unknown internal error");
			}
		}
		catch (...)
		{
			// 오류 JSON을 만드는 중에도 실패(메모리 부족 등)하면 고정 문자열
			code = RcInternal;
			json.clear();
		}

		constexpr std::string_view fallbackJson = "{\"code\":\"internal\",\"message\":\"internal error\",\"details\":[]}";
		*outJson = AllocateString(json.empty() ? fallbackJson : std::string_view(json));
		return code;
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

// ---- 데이터 API: 반환 문자열은 DLL이 malloc — 호출자가 O2un_FreeString으로 해제 ----
int32_t O2un_GetCapabilities(char** outJson)
{
	return RunDataApi("O2un_GetCapabilities", outJson, [&](std::string& out) { return DataApi::GetCapabilities(out); });
}

int32_t O2un_ReadSheet(const char* xlsxPath, const char* sheet, int32_t offset, int32_t limit, char** outJson)
{
	return RunDataApi("O2un_ReadSheet", outJson, [&](std::string& out) { return DataApi::ReadSheet(xlsxPath, sheet, offset, limit, out); });
}

int32_t O2un_AppendRows(const char* xlsxPath, const char* sheet, const char* rowsJson, int32_t dryRun, char** outJson)
{
	return RunDataApi("O2un_AppendRows", outJson, [&](std::string& out) { return DataApi::AppendRows(xlsxPath, sheet, rowsJson, dryRun, out); });
}

int32_t O2un_CreateSheet(const char* root, const char* schemaJson, int32_t dryRun, char** outJson)
{
	return RunDataApi("O2un_CreateSheet", outJson, [&](std::string& out) { return DataApi::CreateSheet(root, schemaJson, dryRun, out); });
}

void O2un_FreeString(char* s)
{
	std::free(s);   // NULL 허용
}
