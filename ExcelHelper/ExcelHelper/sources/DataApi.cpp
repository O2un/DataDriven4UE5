// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
module;
#include "OpenXLSX.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

module IDataApi;
import ICellText;
import ICodeGenerator;
import IJsonParser;
import ILogger;
import IStringHelper;

namespace fs = std::filesystem;
using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

namespace
{
	// ---------- 반환 코드 (ExcelHelper.h 주석과 같다) ----------
	constexpr int32_t RcOk = 0;
	constexpr int32_t RcValidation = 1;
	constexpr int32_t RcInvalidArgument = -1;
	constexpr int32_t RcFileNotFound = -2;
	constexpr int32_t RcSheetNotFound = -3;
	constexpr int32_t RcIo = -4;
	constexpr int32_t RcBadJson = -5;

	constexpr size_t MaxStringBytes = 1024;
	constexpr size_t MaxColumns = 64;
	constexpr size_t MaxDetails = 50;
	constexpr uint32_t MaxExcelRow = 1048576;
	constexpr uint32_t FirstDataRow = 3;

	// ---------- JSON 출력 ----------
	void WriteString(JsonWriter& writer, std::string_view text)
	{
		writer.String(text.data(), static_cast<rapidjson::SizeType>(text.size()));
	}

	void WriteKey(JsonWriter& writer, std::string_view key)
	{
		writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.size()));
	}

	struct Detail
	{
		int64_t row = -1;            // 입력 배열 인덱스. -1이면 행과 무관 (스키마, 시트 상태)
		std::string column;          // 열 이름 (없으면 생략)
		std::string code;
		std::string message;
		uint32_t excelRow = 0;       // 시트 상태 오류의 엑셀 행 번호 (0이면 생략)
	};

	class Failures
	{
	public:
		void Add(int64_t row, std::string_view column, std::string_view code, std::string message, uint32_t excelRow = 0)
		{
			++_total;
			if (_details.size() < MaxDetails)
			{
				_details.push_back(Detail{ row, std::string(column), std::string(code), std::move(message), excelRow });
			}
		}
		bool Empty() const { return 0 == _total; }
		size_t Total() const { return _total; }
		const std::vector<Detail>& Details() const { return _details; }

	private:
		std::vector<Detail> _details;
		size_t _total = 0;
	};

	std::string BuildErrorJson(std::string_view code, std::string_view message, const Failures* failures)
	{
		rapidjson::StringBuffer buffer;
		JsonWriter writer(buffer);
		writer.StartObject();
		WriteKey(writer, "code");
		WriteString(writer, code);
		WriteKey(writer, "message");
		WriteString(writer, message);
		WriteKey(writer, "details");
		writer.StartArray();
		if (failures)
		{
			for (const auto& detail : failures->Details())
			{
				writer.StartObject();
				if (0 <= detail.row)
				{
					WriteKey(writer, "row");
					writer.Int64(detail.row);
				}
				if (false == detail.column.empty())
				{
					WriteKey(writer, "column");
					WriteString(writer, detail.column);
				}
				if (0 != detail.excelRow)
				{
					WriteKey(writer, "excelRow");
					writer.Uint(detail.excelRow);
				}
				WriteKey(writer, "code");
				WriteString(writer, detail.code);
				WriteKey(writer, "message");
				WriteString(writer, detail.message);
				writer.EndObject();
			}
		}
		writer.EndArray();
		if (failures && failures->Details().size() < failures->Total())
		{
			WriteKey(writer, "totalDetails");
			writer.Uint64(failures->Total());
		}
		writer.EndObject();
		return std::string(buffer.GetString(), buffer.GetSize());
	}

	int32_t Fail(std::string& out, int32_t returnCode, std::string_view code, std::string_view message, const Failures* failures = nullptr)
	{
		out = BuildErrorJson(code, message, failures);
		return returnCode;
	}

	// ---------- 입력 ----------
	std::optional<std::string_view> ToText(const char* text)
	{
		if (nullptr == text)
		{
			return std::nullopt;
		}
		const std::string_view view(text);
		if (false == CommonHelper::IsValidUtf8(view))
		{
			return std::nullopt;
		}
		return view;
	}

	// UTF-8 절대 경로 -> fs::path (ANSI 변환 없이)
	std::optional<fs::path> ToAbsolutePath(const char* utf8)
	{
		const auto text = ToText(utf8);
		if (false == text.has_value() || text->empty())
		{
			return std::nullopt;
		}
		const std::u8string u8(reinterpret_cast<const char8_t*>(text->data()), text->size());
		fs::path path(u8);
		if (false == path.is_absolute())
		{
			return std::nullopt;
		}
		return path.lexically_normal();
	}

	// OpenXLSX는 경로를 UTF-8 std::string으로 받는다 (내부에서 nowide 사용)
	std::string ToUtf8(const fs::path& path)
	{
		const std::u8string u8 = path.u8string();
		return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
	}

	std::string LastErrorText()
	{
		return std::format("Win32 error {}", static_cast<unsigned long>(GetLastError()));
	}

	// ---------- 컬럼 타입 ----------
	enum class ColumnType { Int, Long, Float, Double, Bool, String, Name, Text, Key, Unsupported };

	ColumnType ToColumnType(std::string_view typeName)
	{
		static constexpr std::pair<std::string_view, ColumnType> Table[] = {
			{ "int", ColumnType::Int }, { "long", ColumnType::Long }, { "float", ColumnType::Float }, { "double", ColumnType::Double },
			{ "bool", ColumnType::Bool }, { "string", ColumnType::String }, { "name", ColumnType::Name }, { "text", ColumnType::Text },
			{ "key", ColumnType::Key },
		};
		// 지원 여부는 생성기 표(CodeGen::SupportedColumnTypes)가 단일 원천이다
		const bool supported = std::ranges::any_of(CodeGen::SupportedColumnTypes, [&](const CodeGen::ColumnTypeInfo& info) { return CommonHelper::Equals(typeName, info.excelType); });
		if (false == supported)
		{
			return ColumnType::Unsupported;
		}
		for (const auto& [name, type] : Table)
		{
			if (CommonHelper::Equals(typeName, name))
			{
				return type;
			}
		}
		return ColumnType::Unsupported;
	}

	std::string CanonicalTypeName(std::string_view typeName)
	{
		for (const auto& info : CodeGen::SupportedColumnTypes)
		{
			if (CommonHelper::Equals(typeName, info.excelType))
			{
				return std::string(info.excelType);
			}
		}
		return std::string(typeName);
	}

	std::string SupportedTypeList()
	{
		std::string list;
		for (const auto& info : CodeGen::SupportedColumnTypes)
		{
			if (false == list.empty()) list += ", ";
			list += info.excelType;
		}
		return list;
	}

	bool IsStringType(ColumnType type)
	{
		return type == ColumnType::String || type == ColumnType::Name || type == ColumnType::Text;
	}

	bool IsIntegerType(ColumnType type)
	{
		return type == ColumnType::Int || type == ColumnType::Long || type == ColumnType::Key;
	}

	CellText::NumericColumn ToNumericColumn(const std::string& name, ColumnType type)
	{
		if (name == "id" || name == "group")
		{
			return CellText::NumericColumn::Int32;
		}
		switch (type)
		{
		case ColumnType::Int:
		case ColumnType::Key: return CellText::NumericColumn::Int32;
		case ColumnType::Long: return CellText::NumericColumn::Int64;
		case ColumnType::Float: return CellText::NumericColumn::Float;
		case ColumnType::Double: return CellText::NumericColumn::Double;
		default: return CellText::NumericColumn::None;
		}
	}

	// ---------- 숫자 해석 ----------
	std::optional<int64_t> ParseInt64(std::string_view text)
	{
		int64_t value = 0;
		const char* end = text.data() + text.size();
		const auto [parsed, ec] = std::from_chars(text.data(), end, value);
		if (ec == std::errc{} && parsed == end && false == text.empty())
		{
			return value;
		}
		return std::nullopt;
	}

	std::optional<double> ParseDouble(std::string_view text)
	{
		double value = 0.0;
		const char* end = text.data() + text.size();
		const auto [parsed, ec] = std::from_chars(text.data(), end, value);
		if (ec == std::errc{} && parsed == end && false == text.empty() && std::isfinite(value))
		{
			return value;
		}
		return std::nullopt;
	}

	// 정수 열 텍스트 -> 정수 (소수부가 있는 실수 원문은 해석 실패)
	std::optional<int64_t> ParseIntegerText(std::string_view text)
	{
		if (auto value = ParseInt64(text))
		{
			return value;
		}
		if (auto real = ParseDouble(text))
		{
			if (std::trunc(*real) == *real && -9223372036854775808.0 <= *real && *real < 9223372036854775808.0)
			{
				return static_cast<int64_t>(*real);
			}
		}
		return std::nullopt;
	}

	// ---------- 시트 읽기 (로더 JsonParser::ExcelToJson과 같은 규칙) ----------
	struct Column
	{
		uint16_t index = 0;         // 엑셀 열 번호 (1부터)
		std::string name;           // 1행 이름 (밑줄 없이)
		std::string typeName;       // 2행 타입 원문
		ColumnType type = ColumnType::Unsupported;
	};

	struct DataRow
	{
		uint32_t excelRow = 0;
		std::vector<std::string> texts;      // 로더가 내보내는 문자열 (열 순서)
		std::vector<std::string> invalidRaw; // 비어 있지 않으면 로더가 0으로 바꾼 원문
	};

	using RowKey = std::pair<int64_t, int64_t>;   // (group, id)

	struct SheetScan
	{
		std::vector<Column> columns;
		int idColumn = -1;            // columns 인덱스
		int groupColumn = -1;
		std::vector<DataRow> rows;    // 로더가 읽는 행
		std::set<RowKey> keys;        // 기존 키 (칸 수가 안 맞아 로더가 버리는 행 포함)
		uint32_t appendAt = FirstDataRow;
		uint32_t lastDataRow = 0;     // 마지막으로 읽힌 데이터 행 (서식 복사용, 0이면 없음)
		uint32_t trailingDataRow = 0; // 추가 위치 뒤에 값이 있는 행 (0이면 없음)
	};

	bool CellHasValue(OpenXLSX::XLRow& row, uint16_t column)
	{
		auto cell = row.findCell(column);
		return false == cell.empty() && cell.value().type() != OpenXLSX::XLValueType::Empty;
	}

	bool RowHasValue(OpenXLSX::XLRow& row)
	{
		const uint16_t cellCount = row.cellCount();
		for (uint16_t column = 1; column <= cellCount; ++column)
		{
			if (CellHasValue(row, column))
			{
				return true;
			}
		}
		return false;
	}

	// 1행 이름 / 2행 타입. 실패하면 사유
	std::optional<std::string> ReadHeader(const OpenXLSX::XLWorksheet& sheet, SheetScan& scan)
	{
		auto namesRow = sheet.row(1);
		auto typesRow = sheet.row(2);
		const uint16_t count = namesRow.cellCount();
		if (0 == count)
		{
			return std::string("row 1 (column names) is empty");
		}

		std::set<std::string> names;
		for (uint16_t index = 1; index <= count; ++index)
		{
			auto nameCell = namesRow.findCell(index);
			if (nameCell.empty() || nameCell.value().type() != OpenXLSX::XLValueType::String || nameCell.value().getString().empty())
			{
				return std::format("column name cell {}1 is empty or not text", OpenXLSX::XLCellReference::columnAsString(index));
			}

			Column column;
			column.index = index;
			column.name = nameCell.value().getString();
			auto typeCell = typesRow.findCell(index);
			if (false == typeCell.empty() && typeCell.value().type() == OpenXLSX::XLValueType::String)
			{
				column.typeName = typeCell.value().getString();
			}
			column.type = ToColumnType(column.typeName);

			if (false == names.insert(column.name).second)
			{
				return std::format("duplicate column name '{}'", column.name);
			}
			if (column.name == "id") scan.idColumn = static_cast<int>(scan.columns.size());
			if (column.name == "group") scan.groupColumn = static_cast<int>(scan.columns.size());
			scan.columns.push_back(std::move(column));
		}
		return std::nullopt;
	}

	std::optional<RowKey> KeyFromTexts(const SheetScan& scan, const std::vector<std::string>& texts)
	{
		if (scan.idColumn < 0)
		{
			return std::nullopt;
		}
		const auto id = ParseIntegerText(texts[scan.idColumn]);
		std::optional<int64_t> group = int64_t{ 0 };
		if (0 <= scan.groupColumn && false == texts[scan.groupColumn].empty())
		{
			group = ParseIntegerText(texts[scan.groupColumn]);
		}
		if (false == id.has_value() || false == group.has_value())
		{
			return std::nullopt;
		}
		return RowKey{ *group, *id };
	}

	std::vector<std::string> ReadTexts(OpenXLSX::XLRow& row, const SheetScan& scan, std::vector<std::string>* invalidRaw)
	{
		std::vector<std::string> texts;
		texts.reserve(scan.columns.size());
		for (const auto& column : scan.columns)
		{
			auto cell = row.findCell(column.index);
			if (cell.empty())
			{
				texts.emplace_back();
				if (invalidRaw) invalidRaw->emplace_back();
				continue;
			}
			std::string reason;
			std::string raw;
			texts.push_back(CellText::LoaderCellText(cell, ToNumericColumn(column.name, column.type), &reason, &raw));
			if (invalidRaw) invalidRaw->push_back(reason.empty() ? std::string() : raw);
		}
		return texts;
	}

	void ScanRows(const OpenXLSX::XLWorksheet& sheet, SheetScan& scan)
	{
		const uint32_t rowLimit = sheet.rowCount();
		const uint16_t keyCount = static_cast<uint16_t>(scan.columns.size());
		uint32_t stop = 0;
		for (uint32_t excelRow = FirstDataRow; excelRow <= rowLimit; ++excelRow)
		{
			auto row = sheet.row(excelRow);
			const uint16_t cellCount = row.cellCount();
			if (0 == cellCount)
			{
				stop = excelRow;
				break;
			}

			if (keyCount != cellCount)
			{
				// 로더는 이 행을 버린다(경고). 키는 남아 있으므로 중복 검사에는 넣는다
				if (const auto key = KeyFromTexts(scan, ReadTexts(row, scan, nullptr)))
				{
					scan.keys.insert(*key);
				}
				continue;
			}

			auto firstCell = row.findCell(1);
			if (firstCell.empty() || firstCell.value().type() == OpenXLSX::XLValueType::Empty)
			{
				stop = excelRow;
				break;
			}
			if (firstCell.value().type() == OpenXLSX::XLValueType::String && firstCell.value().getString().starts_with("#"))
			{
				continue;
			}

			DataRow dataRow;
			dataRow.excelRow = excelRow;
			dataRow.texts = ReadTexts(row, scan, &dataRow.invalidRaw);
			if (const auto key = KeyFromTexts(scan, dataRow.texts))
			{
				scan.keys.insert(*key);
			}
			scan.lastDataRow = excelRow;
			scan.rows.push_back(std::move(dataRow));
		}

		if (0 == stop)
		{
			stop = std::max(FirstDataRow, rowLimit + 1);
		}
		scan.appendAt = stop;

		// 추가 위치(첫 칸이 빈 행)부터 끝까지 값이 있으면 추가 위치가 모호하다 (그 행을 덮어쓸 수도 있다)
		for (uint32_t excelRow = stop; excelRow <= rowLimit; ++excelRow)
		{
			auto row = sheet.row(excelRow);
			if (RowHasValue(row))
			{
				scan.trailingDataRow = excelRow;
				break;
			}
		}
	}

	// ---------- 통합문서 열기 ----------
	struct OpenedSheet
	{
		OpenXLSX::XLDocument doc;
		OpenXLSX::XLWorksheet sheet;
	};

	// 반환: RcOk 또는 실패 코드(out 채움)
	int32_t OpenSheet(const fs::path& path, std::string_view sheetName, OpenedSheet& opened, std::string& out)
	{
		std::error_code ec;
		if (false == fs::is_regular_file(path, ec))
		{
			return Fail(out, RcFileNotFound, "file_not_found", "workbook file does not exist");
		}
		try
		{
			opened.doc.open(ToUtf8(path));
		}
		catch (const std::exception& e)
		{
			return Fail(out, RcIo, "io_error", std::format("failed to open workbook: {}", CommonHelper::ToUtf8FromSystem(e.what())));
		}

		auto workbook = opened.doc.workbook();
		const std::string name(sheetName);
		if (name.starts_with("#") || false == workbook.worksheetExists(name))
		{
			return Fail(out, RcSheetNotFound, "sheet_not_found", std::format("sheet '{}' not found in workbook", name));
		}
		opened.sheet = workbook.worksheet(name);
		return RcOk;
	}

	// ---------- 행 검증 (§4.4) ----------
	struct CellPlan
	{
		enum class Kind { Integer, Real, Boolean, Text } kind = Kind::Text;
		int64_t integer = 0;
		double real = 0.0;
		bool boolean = false;
		std::string text;
		std::string expected;   // 로더가 다시 읽었을 때 나와야 하는 문자열
	};

	struct RowPlan
	{
		std::vector<CellPlan> cells;   // columns 순서
	};

	CellPlan IntegerPlan(int64_t value)
	{
		CellPlan plan;
		plan.kind = CellPlan::Kind::Integer;
		plan.integer = value;
		plan.expected = std::to_string(value);
		return plan;
	}

	// 실수 쓰기 계획. 엑셀에 쓰는 원문은 OpenXLSX(pugixml)의 "%.17g"이다
	std::optional<CellPlan> RealPlan(double value, std::string& error)
	{
		// 정수 값은 정수 셀로 쓴다 (원문 "5" -> 로더 "5", 지수 표기 없음)
		if (std::trunc(value) == value && -9223372036854775808.0 <= value && value < 9223372036854775808.0)
		{
			return IntegerPlan(static_cast<int64_t>(value));
		}

		char buffer[64]{};
		std::snprintf(buffer, sizeof(buffer), "%.17g", value);
		const std::string_view written(buffer);
		if (std::string_view::npos != written.find_first_of("eE") && std::string_view::npos == written.find('.'))
		{
			// '.' 없는 지수 표기(1e+20)는 OpenXLSX가 정수로 잘못 읽어 로더가 0으로 넣는다
			error = std::format("value {} would be stored as '{}' which the loader cannot read back exactly", CellText::DoubleToLoaderText(value), written);
			return std::nullopt;
		}

		CellPlan plan;
		plan.kind = CellPlan::Kind::Real;
		plan.real = value;
		plan.expected = CellText::DoubleToLoaderText(value);
		return plan;
	}

	const char* JsonTypeName(const rapidjson::Value& value)
	{
		if (value.IsNull()) return "null";
		if (value.IsBool()) return "boolean";
		if (value.IsString()) return "string";
		if (value.IsNumber()) return value.IsDouble() ? "number" : "integer";
		if (value.IsArray()) return "array";
		return "object";
	}

	// 한 셀 검증. 통과하면 계획을 돌려준다
	std::optional<CellPlan> ValidateValue(int64_t rowIndex, const Column& column, const rapidjson::Value& value, Failures& failures)
	{
		const std::string& name = column.name;
		auto mismatch = [&](std::string_view expected) {
			failures.Add(rowIndex, name, "type_mismatch", std::format("column '{}' ({}) expects {}, got {}", name, column.typeName, expected, JsonTypeName(value)));
			return std::optional<CellPlan>{};
		};
		auto outOfRange = [&](std::string message) {
			failures.Add(rowIndex, name, "out_of_range", std::move(message));
			return std::optional<CellPlan>{};
		};

		switch (column.type)
		{
		case ColumnType::Int:
		case ColumnType::Key:
			if (value.IsInt()) return IntegerPlan(value.GetInt());
			if (value.IsInt64() || value.IsUint64()) return outOfRange(std::format("column '{}' ({}) must be within int32 range (-2147483648..2147483647); use type long for larger integers", name, column.typeName));
			return mismatch("a JSON integer");

		case ColumnType::Long:
			if (value.IsInt64()) return IntegerPlan(value.GetInt64());
			if (value.IsUint64()) return outOfRange(std::format("column '{}' (long) must be within int64 range", name));
			return mismatch("a JSON integer");

		case ColumnType::Float:
		case ColumnType::Double:
		{
			if (false == value.IsNumber()) return mismatch("a JSON number");
			if (value.IsInt64()) return IntegerPlan(value.GetInt64());
			const double number = value.GetDouble();
			if (false == std::isfinite(number)) return outOfRange(std::format("column '{}' must be a finite number", name));
			if (column.type == ColumnType::Float && FLT_MAX < std::fabs(number)) return outOfRange(std::format("column '{}' (float) must be within float range", name));
			std::string error;
			if (auto plan = RealPlan(number, error)) return plan;
			return outOfRange(std::format("column '{}': {}", name, error));
		}

		case ColumnType::Bool:
		{
			if (false == value.IsBool()) return mismatch("true or false");
			CellPlan plan;
			plan.kind = CellPlan::Kind::Boolean;
			plan.boolean = value.GetBool();
			plan.expected = plan.boolean ? "true" : "false";
			return plan;
		}

		case ColumnType::String:
		case ColumnType::Name:
		case ColumnType::Text:
		{
			if (false == value.IsString()) return mismatch("a JSON string");
			const std::string_view text(value.GetString(), value.GetStringLength());
			if (text.empty())
			{
				failures.Add(rowIndex, name, "empty_value", std::format("column '{}' must not be empty", name));
				return std::nullopt;
			}
			if (MaxStringBytes < text.size()) return outOfRange(std::format("column '{}' must be at most {} UTF-8 bytes (got {})", name, MaxStringBytes, text.size()));
			if (std::string_view::npos != text.find('\0')) return outOfRange(std::format("column '{}' must not contain NUL", name));
			if (std::ranges::any_of(text, [](char c) { return 0 <= c && c < 0x20 && c != '\t' && c != '\n' && c != '\r'; }))
			{
				return outOfRange(std::format("column '{}' must not contain control characters", name));
			}
			if (column.type == ColumnType::Name && std::string_view::npos != text.find_first_of("\r\n"))
			{
				return outOfRange(std::format("column '{}' (name) must not contain line breaks", name));
			}
			CellPlan plan;
			plan.kind = CellPlan::Kind::Text;
			plan.text = std::string(text);
			plan.expected = plan.text;
			return plan;
		}

		default:
			failures.Add(rowIndex, name, "unsupported_type", std::format("column '{}' has unsupported type '{}'", name, column.typeName));
			return std::nullopt;
		}
	}

	// rows 배열 전체 검증. 통과하면 plans를 채운다
	void ValidateRows(const rapidjson::Value& rows, const SheetScan& scan, std::vector<RowPlan>& plans, Failures& failures)
	{
		std::set<RowKey> proposalKeys;
		for (rapidjson::SizeType ii = 0; ii < rows.Size(); ++ii)
		{
			const int64_t rowIndex = ii;
			const auto& row = rows[ii];
			if (false == row.IsObject())
			{
				failures.Add(rowIndex, "", "type_mismatch", std::format("row {} must be a JSON object, got {}", ii, JsonTypeName(row)));
				continue;
			}

			for (auto member = row.MemberBegin(); member != row.MemberEnd(); ++member)
			{
				const std::string_view key(member->name.GetString(), member->name.GetStringLength());
				const bool known = std::ranges::any_of(scan.columns, [&](const Column& column) { return column.name == key; });
				if (false == known)
				{
					failures.Add(rowIndex, key, "unknown_column", std::format("column '{}' does not exist in the sheet", key));
				}
			}

			RowPlan plan;
			bool rowOk = true;
			for (size_t columnIndex = 0; columnIndex < scan.columns.size(); ++columnIndex)
			{
				const Column& column = scan.columns[columnIndex];
				const auto member = row.FindMember(rapidjson::StringRef(column.name.data(), static_cast<rapidjson::SizeType>(column.name.size())));
				if (member == row.MemberEnd())
				{
					if (static_cast<int>(columnIndex) == scan.groupColumn)
					{
						plan.cells.push_back(IntegerPlan(0));   // group만 생략 가능 -> 0
						continue;
					}
					failures.Add(rowIndex, column.name, "missing_column", std::format("column '{}' is required", column.name));
					rowOk = false;
					plan.cells.emplace_back();
					continue;
				}

				auto cell = ValidateValue(rowIndex, column, member->value, failures);
				if (false == cell.has_value())
				{
					rowOk = false;
					plan.cells.emplace_back();
					continue;
				}
				plan.cells.push_back(std::move(*cell));
			}
			if (false == rowOk)
			{
				continue;
			}

			// 첫 번째 열이 '#'로 시작하면 로더가 주석 행으로 건너뛴다
			if (false == plan.cells.empty() && plan.cells[0].kind == CellPlan::Kind::Text && plan.cells[0].text.starts_with("#"))
			{
				failures.Add(rowIndex, scan.columns[0].name, "comment_prefix", std::format("first column '{}' must not start with '#' (the loader skips such rows as comments)", scan.columns[0].name));
			}

			if (0 <= scan.idColumn)
			{
				const int64_t id = plan.cells[scan.idColumn].integer;
				const int64_t group = 0 <= scan.groupColumn ? plan.cells[scan.groupColumn].integer : 0;
				const RowKey key{ group, id };
				if (0 == group && 0 == id)
				{
					failures.Add(rowIndex, "id", "invalid_key", "key (group=0, id=0) is not a valid key");
				}
				else if (scan.keys.contains(key))
				{
					failures.Add(rowIndex, "id", "duplicate_key_existing", std::format("key (group={}, id={}) already exists in the sheet", group, id));
				}
				else if (false == proposalKeys.insert(key).second)
				{
					failures.Add(rowIndex, "id", "duplicate_key_in_proposal", std::format("key (group={}, id={}) appears more than once in the proposal", group, id));
				}
			}
			plans.push_back(std::move(plan));
		}
	}

	// ---------- 쓰기 / 왕복 확인 ----------
	void WriteRows(OpenXLSX::XLWorksheet& sheet, const SheetScan& scan, const std::vector<RowPlan>& plans, uint32_t startRow)
	{
		for (size_t rowIndex = 0; rowIndex < plans.size(); ++rowIndex)
		{
			const uint32_t excelRow = startRow + static_cast<uint32_t>(rowIndex);
			for (size_t columnIndex = 0; columnIndex < scan.columns.size(); ++columnIndex)
			{
				const uint16_t columnNumber = scan.columns[columnIndex].index;
				auto cell = sheet.cell(excelRow, columnNumber);

				// 기존 데이터 행의 셀 서식을 따른다 (Q4: 숫자는 숫자 셀, 문자열은 공유 문자열 셀)
				if (0 != scan.lastDataRow)
				{
					auto source = sheet.findCell(scan.lastDataRow, columnNumber);
					if (false == source.empty())
					{
						cell.setCellFormat(source.cellFormat());
					}
				}

				const CellPlan& plan = plans[rowIndex].cells[columnIndex];
				switch (plan.kind)
				{
				case CellPlan::Kind::Integer: cell.value() = plan.integer; break;
				case CellPlan::Kind::Real: cell.value() = plan.real; break;
				case CellPlan::Kind::Boolean: cell.value() = plan.boolean; break;
				case CellPlan::Kind::Text: cell.value() = plan.text; break;
				}
			}
		}
	}

	// 저장한 파일을 로더(JsonParser)로 다시 읽어 추가한 행이 그대로 나오는지 확인한다
	void VerifyRoundTrip(const fs::path& file, const std::string& sheetName, const SheetScan& scan, const std::vector<RowPlan>& plans, size_t existingRows, Failures& failures)
	{
		OpenXLSX::XLDocument doc;
		doc.open(ToUtf8(file));
		std::string json;
		{
			Logger::MuteScope mute;   // 기존 행의 경고를 다시 내보내지 않는다
			JsonParser parser;
			json = parser.ExcelToJson(doc.workbook().worksheet(sheetName));
		}
		doc.close();

		rapidjson::Document loaded;
		loaded.Parse(json.c_str());
		const size_t expectedRows = existingRows + plans.size();
		if (loaded.HasParseError() || false == loaded.IsArray() || loaded.Size() != expectedRows)
		{
			failures.Add(-1, "", "round_trip_mismatch", std::format("loader read {} rows after writing, expected {}", loaded.IsArray() ? loaded.Size() : 0, expectedRows));
			return;
		}

		for (size_t rowIndex = 0; rowIndex < plans.size(); ++rowIndex)
		{
			const auto& object = loaded[static_cast<rapidjson::SizeType>(existingRows + rowIndex)];
			for (size_t columnIndex = 0; columnIndex < scan.columns.size(); ++columnIndex)
			{
				const Column& column = scan.columns[columnIndex];
				const rapidjson::Value* value = nullptr;
				if (static_cast<int>(columnIndex) == scan.idColumn || static_cast<int>(columnIndex) == scan.groupColumn)
				{
					const char* keyName = static_cast<int>(columnIndex) == scan.idColumn ? "_id" : "_group";
					if (object.HasMember("_key") && object["_key"].HasMember(keyName))
					{
						value = &object["_key"][keyName];
					}
				}
				else
				{
					const std::string key = "_" + column.name;
					const auto member = object.FindMember(key.c_str());
					if (member != object.MemberEnd())
					{
						value = &member->value;
					}
				}

				const std::string& expected = plans[rowIndex].cells[columnIndex].expected;
				const std::string actual = (value && value->IsString()) ? std::string(value->GetString(), value->GetStringLength()) : std::string("<missing>");
				if (actual != expected)
				{
					failures.Add(static_cast<int64_t>(rowIndex), column.name, "round_trip_mismatch", std::format("column '{}': wrote '{}' but the loader reads '{}'", column.name, expected, actual));
				}
			}
		}
	}

	// 실패 시 지우는 임시 파일
	class TempFile
	{
	public:
		explicit TempFile(fs::path path) : _path(std::move(path)) {}
		~TempFile()
		{
			if (false == _released)
			{
				std::error_code ec;
				fs::remove(_path, ec);
			}
		}
		TempFile(const TempFile&) = delete;
		TempFile& operator=(const TempFile&) = delete;
		const fs::path& Path() const { return _path; }
		void Release() { _released = true; }

	private:
		fs::path _path;
		bool _released = false;
	};

	fs::path DryRunTempPath()
	{
		std::random_device device;
		const auto name = std::format("~&o2un-dryrun-{}-{:08x}.xlsx", GetCurrentProcessId(), device());
		return fs::temp_directory_path() / name;
	}

	// temp -> target 원자적 교체. 실패하면 사유
	std::optional<std::string> MoveIntoPlace(const fs::path& temp, const fs::path& target, bool replaceExisting)
	{
		DWORD flags = MOVEFILE_WRITE_THROUGH;
		if (replaceExisting) flags |= MOVEFILE_REPLACE_EXISTING;
		if (MoveFileExW(temp.c_str(), target.c_str(), flags))
		{
			return std::nullopt;
		}
		return LastErrorText();
	}

	void WriteNormalizedRows(JsonWriter& writer, const SheetScan& scan, const std::vector<RowPlan>& plans)
	{
		writer.StartArray();
		for (const auto& plan : plans)
		{
			writer.StartObject();
			for (size_t columnIndex = 0; columnIndex < scan.columns.size(); ++columnIndex)
			{
				const CellPlan& cell = plan.cells[columnIndex];
				WriteKey(writer, scan.columns[columnIndex].name);
				switch (cell.kind)
				{
				case CellPlan::Kind::Integer: writer.Int64(cell.integer); break;
				case CellPlan::Kind::Real: writer.Double(cell.real); break;
				case CellPlan::Kind::Boolean: writer.Bool(cell.boolean); break;
				case CellPlan::Kind::Text: WriteString(writer, cell.text); break;
				}
			}
			writer.EndObject();
		}
		writer.EndArray();
	}

	// ---------- ReadSheet 값 ----------
	// 로더 문자열을 열 타입으로 해석해 쓴다. 실패하면 false (호출부가 원문 문자열을 쓴다)
	bool WriteTypedValue(JsonWriter& writer, ColumnType type, const std::string& text)
	{
		switch (type)
		{
		case ColumnType::Int:
		case ColumnType::Key:
		case ColumnType::Long:
		{
			const auto value = ParseIntegerText(text);
			if (false == value.has_value()) return false;
			if (type != ColumnType::Long && (*value < INT32_MIN || INT32_MAX < *value)) return false;
			writer.Int64(*value);
			return true;
		}
		case ColumnType::Float:
		case ColumnType::Double:
		{
			const auto value = ParseDouble(text);
			if (false == value.has_value()) return false;
			if (type == ColumnType::Float && FLT_MAX < std::fabs(*value)) return false;
			writer.Double(*value);
			return true;
		}
		case ColumnType::Bool:
			if (CommonHelper::Equals(text, "true")) { writer.Bool(true); return true; }
			if (CommonHelper::Equals(text, "false")) { writer.Bool(false); return true; }
			return false;
		default:
			WriteString(writer, text);   // 문자열류, 지원하지 않는 타입은 원문 그대로
			return true;
		}
	}

	// ---------- 스키마 검증 (CreateSheet) ----------
	// 시트 이름은 Excel 한도 31자, 열 이름은 64자
	constexpr size_t MaxSheetNameLength = 31;
	constexpr size_t MaxColumnNameLength = 64;

	bool IsValidIdentifier(std::string_view name, size_t maxLength)
	{
		if (name.empty() || maxLength < name.size()) return false;
		const auto isAlpha = [](char c) { return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'); };
		const auto isDigit = [](char c) { return '0' <= c && c <= '9'; };
		if (false == isAlpha(name[0])) return false;
		return std::ranges::all_of(name, [&](char c) { return isAlpha(c) || isDigit(c) || c == '_'; });
	}

	// root 아래 통합문서에서 같은 이름(대소문자 무시)의 시트를 찾는다. 로더와 같은 파일 규칙(~&, # 접두사 제외, .xlsx/.xlsm)
	std::optional<fs::path> FindSheetUnderRoot(const fs::path& root, std::string_view sheetName)
	{
		std::error_code ec;
		for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
		{
			const fs::path& path = it->path();
			if (false == it->is_regular_file(ec))
			{
				continue;
			}
			const auto fileName = path.filename().native();
			if (fileName.empty() || fileName.starts_with(L"~&") || fileName.starts_with(L"#"))
			{
				continue;
			}
			const auto ext = path.extension().native();
			if (L".xlsx" != ext && L".xlsm" != ext)
			{
				continue;
			}

			try
			{
				OpenXLSX::XLDocument doc;
				doc.open(ToUtf8(path));
				for (const auto& name : doc.workbook().sheetNames())
				{
					if (false == name.starts_with("#") && CommonHelper::Equals(name, sheetName))
					{
						return path;
					}
				}
			}
			catch (...)
			{
				// 열 수 없는 파일은 로더도 건너뛴다
			}
		}
		return std::nullopt;
	}

	bool ParseJson(const char* text, rapidjson::Document& document, std::string& error)
	{
		document.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseFullPrecisionFlag>(text);
		if (document.HasParseError())
		{
			error = std::format("JSON parse error at offset {}: {}", document.GetErrorOffset(), rapidjson::GetParseError_En(document.GetParseError()));
			return false;
		}
		return true;
	}
}

namespace DataApi
{
	std::string MakeErrorJson(std::string_view code, std::string_view message)
	{
		return BuildErrorJson(code, message, nullptr);
	}

	int32_t GetCapabilities(std::string& out)
	{
		rapidjson::StringBuffer buffer;
		JsonWriter writer(buffer);
		writer.StartObject();
		WriteKey(writer, "abiVersion");
		writer.Int(AbiVersion);
		WriteKey(writer, "types");
		writer.StartArray();
		for (const auto& info : CodeGen::SupportedColumnTypes)
		{
			writer.StartObject();
			WriteKey(writer, "name");
			WriteString(writer, info.excelType);
			WriteKey(writer, "cpp");
			// key 열은 멤버를 만들지 않고 _key(FStaticDataKey)로 간다
			WriteString(writer, info.cppType.empty() ? std::string_view("FStaticDataKey") : info.cppType);
			writer.EndObject();
		}
		writer.EndArray();
		WriteKey(writer, "keyType");
		WriteString(writer, "key");
		WriteKey(writer, "keyColumns");
		writer.StartObject();
		WriteKey(writer, "id");
		WriteString(writer, "required");
		WriteKey(writer, "group");
		WriteString(writer, "optional");
		writer.EndObject();
		WriteKey(writer, "reservedColumnNames");
		writer.StartArray();
		WriteString(writer, "key");
		writer.EndArray();
		WriteKey(writer, "limits");
		writer.StartObject();
		WriteKey(writer, "maxStringBytes");
		writer.Uint64(MaxStringBytes);
		WriteKey(writer, "maxColumns");
		writer.Uint64(MaxColumns);
		writer.EndObject();
		writer.EndObject();
		out.assign(buffer.GetString(), buffer.GetSize());
		return RcOk;
	}

	int32_t ReadSheet(const char* xlsxPath, const char* sheet, int32_t offset, int32_t limit, std::string& out)
	{
		const auto path = ToAbsolutePath(xlsxPath);
		const auto sheetName = ToText(sheet);
		if (false == path.has_value() || false == sheetName.has_value() || sheetName->empty() || offset < 0 || limit < 0)
		{
			return Fail(out, RcInvalidArgument, "invalid_argument", "xlsxPath must be an absolute UTF-8 path, sheet must be non-empty UTF-8, offset and limit must be >= 0");
		}

		OpenedSheet opened;
		if (const int32_t rc = OpenSheet(*path, *sheetName, opened, out); RcOk != rc)
		{
			return rc;
		}

		SheetScan scan;
		if (const auto error = ReadHeader(opened.sheet, scan))
		{
			return Fail(out, RcValidation, "schema_invalid", *error);
		}
		ScanRows(opened.sheet, scan);

		rapidjson::StringBuffer buffer;
		JsonWriter writer(buffer);
		writer.StartObject();
		WriteKey(writer, "sheet");
		WriteString(writer, *sheetName);
		WriteKey(writer, "columns");
		writer.StartArray();
		for (const auto& column : scan.columns)
		{
			writer.StartObject();
			WriteKey(writer, "name");
			WriteString(writer, column.name);
			WriteKey(writer, "type");
			WriteString(writer, column.typeName);
			WriteKey(writer, "supported");
			writer.Bool(column.type != ColumnType::Unsupported);
			writer.EndObject();
		}
		writer.EndArray();
		WriteKey(writer, "dataRowCount");
		writer.Uint64(scan.rows.size());

		std::map<int64_t, int64_t> maxIdByGroup;
		for (const auto& row : scan.rows)
		{
			if (const auto key = KeyFromTexts(scan, row.texts))
			{
				auto [it, inserted] = maxIdByGroup.try_emplace(key->first, key->second);
				if (false == inserted) it->second = std::max(it->second, key->second);
			}
		}
		WriteKey(writer, "maxIdByGroup");
		writer.StartObject();
		for (const auto& [group, id] : maxIdByGroup)
		{
			WriteKey(writer, std::to_string(group));
			writer.Int64(id);
		}
		writer.EndObject();

		WriteKey(writer, "rows");
		writer.StartArray();
		const size_t begin = std::min(scan.rows.size(), static_cast<size_t>(offset));
		const size_t end = std::min(scan.rows.size(), begin + static_cast<size_t>(limit));
		for (size_t rowIndex = begin; rowIndex < end; ++rowIndex)
		{
			const DataRow& row = scan.rows[rowIndex];
			std::vector<std::string_view> invalid;
			writer.StartObject();
			WriteKey(writer, "excelRow");
			writer.Uint(row.excelRow);
			WriteKey(writer, "values");
			writer.StartObject();
			for (size_t columnIndex = 0; columnIndex < scan.columns.size(); ++columnIndex)
			{
				const Column& column = scan.columns[columnIndex];
				WriteKey(writer, column.name);
				const std::string& raw = row.invalidRaw[columnIndex];
				if (false == raw.empty() || false == WriteTypedValue(writer, column.type, row.texts[columnIndex]))
				{
					WriteString(writer, raw.empty() ? row.texts[columnIndex] : raw);   // 해석 실패: 원문 문자열
					invalid.push_back(column.name);
				}
			}
			writer.EndObject();
			if (false == invalid.empty())
			{
				WriteKey(writer, "invalid");
				writer.StartArray();
				for (const auto name : invalid) WriteString(writer, name);
				writer.EndArray();
			}
			writer.EndObject();
		}
		writer.EndArray();

		WriteKey(writer, "nextOffset");
		if (0 < limit && end < scan.rows.size())
		{
			writer.Uint64(end);
		}
		else
		{
			writer.Null();
		}
		writer.EndObject();
		out.assign(buffer.GetString(), buffer.GetSize());
		return RcOk;
	}

	int32_t AppendRows(const char* xlsxPath, const char* sheet, const char* rowsJson, int32_t dryRun, std::string& out)
	{
		const auto path = ToAbsolutePath(xlsxPath);
		const auto sheetName = ToText(sheet);
		if (false == path.has_value() || false == sheetName.has_value() || sheetName->empty() || nullptr == rowsJson)
		{
			return Fail(out, RcInvalidArgument, "invalid_argument", "xlsxPath must be an absolute UTF-8 path, sheet must be non-empty UTF-8, rowsJson must not be NULL");
		}

		rapidjson::Document rows;
		std::string parseError;
		if (false == ParseJson(rowsJson, rows, parseError))
		{
			return Fail(out, RcBadJson, "invalid_json", parseError);
		}
		if (false == rows.IsArray() || rows.Empty())
		{
			return Fail(out, RcBadJson, "invalid_json", "rowsJson must be a non-empty JSON array of row objects");
		}

		OpenedSheet opened;
		if (const int32_t rc = OpenSheet(*path, *sheetName, opened, out); RcOk != rc)
		{
			return rc;
		}

		SheetScan scan;
		if (const auto error = ReadHeader(opened.sheet, scan))
		{
			return Fail(out, RcValidation, "schema_invalid", *error);
		}
		if (scan.idColumn < 0 || scan.columns[scan.idColumn].type != ColumnType::Key || (0 <= scan.groupColumn && scan.columns[scan.groupColumn].type != ColumnType::Key))
		{
			return Fail(out, RcValidation, "schema_invalid", "sheet must have an 'id' column of type key ('group', if present, must be type key)");
		}
		{
			Failures unsupported;
			for (const auto& column : scan.columns)
			{
				if (column.type == ColumnType::Unsupported)
				{
					unsupported.Add(-1, column.name, "unsupported_type", std::format("column '{}' has unsupported type '{}' (supported: {})", column.name, column.typeName, SupportedTypeList()));
				}
			}
			if (false == unsupported.Empty())
			{
				return Fail(out, RcValidation, "sheet_has_unsupported_types", "the sheet has columns with unsupported types", &unsupported);
			}
		}

		ScanRows(opened.sheet, scan);
		if (0 != scan.trailingDataRow)
		{
			Failures state;
			state.Add(-1, "", "non_contiguous_rows", std::format("row {} has data after the first row whose first cell is empty (row {})", scan.trailingDataRow, scan.appendAt), scan.trailingDataRow);
			return Fail(out, RcValidation, "non_contiguous_rows", "the sheet has data after an empty row; the append position is ambiguous", &state);
		}

		std::vector<RowPlan> plans;
		Failures failures;
		ValidateRows(rows, scan, plans, failures);
		if (false == failures.Empty())
		{
			return Fail(out, RcValidation, "validation_failed", std::format("{} problem(s) found in the proposed rows", failures.Total()), &failures);
		}
		const uint32_t firstRow = scan.appendAt;
		const uint32_t lastRow = firstRow + static_cast<uint32_t>(plans.size()) - 1;
		if (MaxExcelRow < lastRow)
		{
			return Fail(out, RcValidation, "too_many_rows", "rows would exceed the Excel row limit");
		}

		// 메모리에서 쓰고 임시 파일로 저장 -> 로더로 다시 읽어 확인 -> (실제 쓰기면) 원본 교체
		WriteRows(opened.sheet, scan, plans, firstRow);
		const bool isDryRun = 0 != dryRun;
		TempFile temp(isDryRun ? DryRunTempPath() : path->parent_path() / (L"~&" + path->filename().native()));
		try
		{
			opened.doc.saveAs(ToUtf8(temp.Path()), OpenXLSX::XLForceOverwrite);
			opened.doc.close();
		}
		catch (const std::exception& e)
		{
			return Fail(out, RcIo, "io_error", std::format("failed to save temporary workbook: {}", CommonHelper::ToUtf8FromSystem(e.what())));
		}

		Failures roundTrip;
		VerifyRoundTrip(temp.Path(), std::string(*sheetName), scan, plans, scan.rows.size(), roundTrip);
		if (false == roundTrip.Empty())
		{
			return Fail(out, RcValidation, "round_trip_mismatch", "the loader would not read the written rows back unchanged", &roundTrip);
		}

		rapidjson::StringBuffer buffer;
		JsonWriter writer(buffer);
		writer.StartObject();
		WriteKey(writer, "rows");
		writer.Uint64(plans.size());
		if (isDryRun)
		{
			WriteKey(writer, "appendAt");
			writer.Uint(firstRow);
			WriteKey(writer, "normalized");
			WriteNormalizedRows(writer, scan, plans);
		}
		else
		{
			if (const auto error = MoveIntoPlace(temp.Path(), *path, true))
			{
				return Fail(out, RcIo, "io_error", std::format("failed to replace the workbook (is it open in Excel?): {}", *error));
			}
			temp.Release();
			WriteKey(writer, "firstRow");
			writer.Uint(firstRow);
			WriteKey(writer, "lastRow");
			writer.Uint(lastRow);
		}
		writer.EndObject();
		out.assign(buffer.GetString(), buffer.GetSize());
		return RcOk;
	}

	int32_t CreateSheet(const char* root, const char* schemaJson, int32_t dryRun, std::string& out)
	{
		const auto rootPath = ToAbsolutePath(root);
		if (false == rootPath.has_value() || nullptr == schemaJson)
		{
			return Fail(out, RcInvalidArgument, "invalid_argument", "root must be an absolute UTF-8 path and schemaJson must not be NULL");
		}

		rapidjson::Document schema;
		std::string parseError;
		if (false == ParseJson(schemaJson, schema, parseError))
		{
			return Fail(out, RcBadJson, "invalid_json", parseError);
		}
		const auto sheetMember = schema.IsObject() ? schema.FindMember("sheet") : schema.MemberEnd();
		const auto columnsMember = schema.IsObject() ? schema.FindMember("columns") : schema.MemberEnd();
		if (false == schema.IsObject() || sheetMember == schema.MemberEnd() || false == sheetMember->value.IsString()
			|| columnsMember == schema.MemberEnd() || false == columnsMember->value.IsArray())
		{
			return Fail(out, RcBadJson, "invalid_json", "schemaJson must be an object with string 'sheet' and array 'columns'");
		}
		const rapidjson::Value* rows = nullptr;
		if (const auto rowsMember = schema.FindMember("rows"); rowsMember != schema.MemberEnd())
		{
			if (false == rowsMember->value.IsArray())
			{
				return Fail(out, RcBadJson, "invalid_json", "'rows' must be an array when present");
			}
			rows = &rowsMember->value;
		}
		for (const auto& column : columnsMember->value.GetArray())
		{
			if (false == column.IsObject() || false == column.HasMember("name") || false == column["name"].IsString()
				|| false == column.HasMember("type") || false == column["type"].IsString())
			{
				return Fail(out, RcBadJson, "invalid_json", "each column must be an object with string 'name' and 'type'");
			}
		}

		std::error_code ec;
		if (false == fs::is_directory(*rootPath, ec))
		{
			return Fail(out, RcFileNotFound, "file_not_found", "root directory does not exist");
		}

		const std::string sheetName(sheetMember->value.GetString(), sheetMember->value.GetStringLength());
		const fs::path target = *rootPath / fs::path(std::u8string(reinterpret_cast<const char8_t*>(sheetName.data()), sheetName.size()) + u8".xlsx");

		Failures failures;
		if (false == IsValidIdentifier(sheetName, MaxSheetNameLength))
		{
			failures.Add(-1, "", "invalid_name", std::format("sheet name '{}' must match ^[A-Za-z][A-Za-z0-9_]{{0,30}}$ (Excel sheet names are at most 31 characters)", sheetName));
		}
		else if (fs::exists(target, ec))
		{
			failures.Add(-1, "", "sheet_exists", std::format("file '{}.xlsx' already exists", sheetName));
		}
		else if (const auto existing = FindSheetUnderRoot(*rootPath, sheetName))
		{
			failures.Add(-1, "", "sheet_exists", std::format("sheet '{}' already exists in '{}'", sheetName, ToUtf8(existing->lexically_relative(*rootPath))));
		}

		SheetScan scan;
		const auto& columns = columnsMember->value;
		if (MaxColumns < columns.Size())
		{
			failures.Add(-1, "", "too_many_columns", std::format("at most {} columns are allowed (got {})", MaxColumns, columns.Size()));
		}
		std::set<std::string> lowerNames;
		for (rapidjson::SizeType ii = 0; ii < columns.Size(); ++ii)
		{
			Column column;
			column.index = static_cast<uint16_t>(ii + 1);
			column.name.assign(columns[ii]["name"].GetString(), columns[ii]["name"].GetStringLength());
			const std::string requestedType(columns[ii]["type"].GetString(), columns[ii]["type"].GetStringLength());
			column.type = ToColumnType(requestedType);
			column.typeName = CanonicalTypeName(requestedType);

			std::string lower = column.name;
			std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			if (false == IsValidIdentifier(column.name, MaxColumnNameLength))
			{
				failures.Add(-1, column.name, "invalid_name", std::format("column name '{}' must match ^[A-Za-z][A-Za-z0-9_]{{0,63}}$", column.name));
			}
			else if (lower == "key")
			{
				failures.Add(-1, column.name, "reserved_name", "column name 'key' is reserved (it would collide with FStaticDataInitStruct::_key)");
			}
			else if (false == lowerNames.insert(lower).second)
			{
				failures.Add(-1, column.name, "duplicate_column", std::format("column name '{}' is duplicated (case-insensitive)", column.name));
			}

			if (column.type == ColumnType::Unsupported)
			{
				std::string message = std::format("column '{}' has unsupported type '{}' (supported: {})", column.name, requestedType, SupportedTypeList());
				static constexpr std::string_view otherIntTypes[] = { "uint", "uint32", "uint64", "int8", "int16", "int32", "int64", "integer" };
				if (std::ranges::any_of(otherIntTypes, [&](std::string_view t) { return CommonHelper::Equals(requestedType, t); }))
				{
					message += "; use int (32-bit) or long (64-bit)";
				}
				failures.Add(-1, column.name, "unsupported_type", std::move(message));
			}
			else if (column.type == ColumnType::Key && column.name != "id" && column.name != "group")
			{
				failures.Add(-1, column.name, "missing_id_column", std::format("only 'id' and 'group' may use type key (column '{}')", column.name));
			}
			else if ((column.name == "id" || column.name == "group") && column.type != ColumnType::Key)
			{
				failures.Add(-1, column.name, "missing_id_column", std::format("column '{}' must be type key", column.name));
			}

			if (column.name == "id") scan.idColumn = static_cast<int>(ii);
			if (column.name == "group") scan.groupColumn = static_cast<int>(ii);
			scan.columns.push_back(std::move(column));
		}
		if (scan.idColumn < 0)
		{
			failures.Add(-1, "", "missing_id_column", "an 'id' column of type key is required");
		}

		std::vector<RowPlan> plans;
		if (failures.Empty() && rows)
		{
			ValidateRows(*rows, scan, plans, failures);
		}
		if (false == failures.Empty())
		{
			return Fail(out, RcValidation, "validation_failed", std::format("{} problem(s) found in the schema", failures.Total()), &failures);
		}

		// 새 통합문서: 1행 이름, 2행 타입, 3행부터 데이터
		const bool isDryRun = 0 != dryRun;
		TempFile temp(isDryRun ? DryRunTempPath() : rootPath.value() / fs::path(u8"~&" + target.filename().u8string()));
		try
		{
			OpenXLSX::XLDocument doc;
			doc.create(ToUtf8(temp.Path()), OpenXLSX::XLForceOverwrite);
			auto workbook = doc.workbook();
			auto worksheet = workbook.worksheet(workbook.sheetNames().front());
			worksheet.setName(sheetName);
			for (const auto& column : scan.columns)
			{
				worksheet.cell(1, column.index).value() = column.name;
				worksheet.cell(2, column.index).value() = column.typeName;
			}
			WriteRows(worksheet, scan, plans, FirstDataRow);
			doc.save();
			doc.close();
		}
		catch (const std::exception& e)
		{
			return Fail(out, RcIo, "io_error", std::format("failed to create temporary workbook: {}", CommonHelper::ToUtf8FromSystem(e.what())));
		}

		Failures roundTrip;
		VerifyRoundTrip(temp.Path(), sheetName, scan, plans, 0, roundTrip);
		if (false == roundTrip.Empty())
		{
			return Fail(out, RcValidation, "round_trip_mismatch", "the loader would not read the sample rows back unchanged", &roundTrip);
		}

		if (false == isDryRun)
		{
			if (const auto error = MoveIntoPlace(temp.Path(), target, false))
			{
				return Fail(out, RcIo, "io_error", std::format("failed to create '{}': {}", ToUtf8(target.filename()), *error));
			}
			temp.Release();
		}

		rapidjson::StringBuffer buffer;
		JsonWriter writer(buffer);
		writer.StartObject();
		WriteKey(writer, "workbook");
		WriteString(writer, ToUtf8(target));
		WriteKey(writer, "sheet");
		WriteString(writer, sheetName);
		WriteKey(writer, "rows");
		writer.Uint64(plans.size());
		writer.EndObject();
		out.assign(buffer.GetString(), buffer.GetSize());
		return RcOk;
	}
}
