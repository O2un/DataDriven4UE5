// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
// O2un_* C ABI(ExcelHelper.h)의 본문. 규칙은 artifacts/design/ai-fill.md §3, 검증 규칙은 §4.4.
// 모든 함수는 결과 JSON을 out에 채우고 반환 코드(0, 1, -1, -2, -3, -4, -5)를 돌려준다. 예외는 호출부(ExcelHelper.cpp)가 -99로 바꾼다.
// 전역 버퍼를 쓰지 않는다 (ExcelLoader 싱글턴과 공유하지 않음).
export module IDataApi;

import <cstdint>;
import <string>;
import <string_view>;

export namespace DataApi
{
	inline constexpr int32_t AbiVersion = 1;

	int32_t GetCapabilities(std::string& out);
	int32_t ReadSheet(const char* xlsxPath, const char* sheet, int32_t offset, int32_t limit, std::string& out);
	int32_t AppendRows(const char* xlsxPath, const char* sheet, const char* rowsJson, int32_t dryRun, std::string& out);
	int32_t CreateSheet(const char* root, const char* schemaJson, int32_t dryRun, std::string& out);

	// {"code":..,"message":..,"details":[]}
	std::string MakeErrorJson(std::string_view code, std::string_view message);
}
