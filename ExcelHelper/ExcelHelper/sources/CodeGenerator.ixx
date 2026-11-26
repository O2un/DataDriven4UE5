// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
module;
#include "OpenXLSX.hpp"
#include <array>
#include <optional>
#include <string_view>
#include <vector>

export module ICodeGenerator;

export namespace CodeGen
{
	struct ColumnTypeInfo
	{
		std::string_view excelType;	// 엑셀 2행 타입 이름 (소문자, 비교는 대소문자 무시)
		std::string_view cppType;	// 생성 멤버의 C++ 타입. 빈 값이면 멤버를 만들지 않는다 (key)
	};

	// 생성기가 지원하는 컬럼 타입의 단일 원천. 추후 GetCapabilities도 이 표를 쓴다 (design/ai-fill.md §7).
	inline constexpr std::array<ColumnTypeInfo, 8> SupportedColumnTypes{ {
		{ "int",    "int32"   },
		{ "float",  "float"   },
		{ "double", "double"  },
		{ "bool",   "bool"    },
		{ "string", "FString" },
		{ "name",   "FName"   },
		{ "text",   "FText"   },
		{ "key",    ""        },
	} };
}

export class CodeGenerator
{
public:
	CodeGenerator() = default;

	void GenerateHeader(const OpenXLSX::XLWorksheet& sheet);
	void SetCodeGeneratePath(std::string_view api, std::string_view path);

private:
	struct GeneratedCodeResult
	{
		std::string properties;
		std::string inits;
		std::string assigns;
		std::vector<std::string> errors;	// 하나라도 있으면 그 시트는 파일을 쓰지 않는다
	};

	std::unordered_map<int, std::string> _templateCache;
	std::string _projectName;
	std::string _codePath;

	void GenerateHeaderUnchecked(const OpenXLSX::XLWorksheet& sheet);	// GenerateHeader가 예외를 막고 호출한다
	std::string_view GetOrLoadTemplate(int resourceID);
	bool CreateStaticData(const OpenXLSX::XLWorksheet& sheet, const GeneratedCodeResult& values);
	bool CreateDataAsset(const OpenXLSX::XLWorksheet& sheet);
	void ReplaceTag(std::string& target, std::string_view tag, std::string_view value);
	GeneratedCodeResult BuildValuesSection(const OpenXLSX::XLWorksheet& sheet);
	std::optional<std::string_view> MapToCppType(std::string_view excelType);
	std::string BuildSupportedTypeList();
	void SaveFile(std::string_view detailPath, std::string_view header, std::string_view cpp);
	void SaveFile(std::string_view path, std::string_view content);
};