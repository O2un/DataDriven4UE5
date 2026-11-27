module;
#include "OpenXLSX.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <format>
#include <cstdint>
#include <optional>
#include <string_view>

module IJsonParser;
import <string>;
import <vector>;
import ICellText;
import ILogger;
import IStringHelper;

using namespace rapidjson;
using namespace CellText;   // CellValueToString, NumericColumn, ToNumericColumn, ReadRawNumberText, FindTooLargeNumber

const char* JsonParser::ExcelToJson(const OpenXLSX::XLWorksheet& sheet)
{
    _buffer.Clear();

    if (false == sheet.valid())
    {
        return "";
    }

    std::vector<std::string> keys;
    std::vector<std::vector<std::string>> rows;

    auto namesRow = sheet.row(1);
    std::vector<uint16_t> keyColumns;
    for (auto& cell : namesRow.cells())
    {
        keys.push_back("_" + cell.value().get<std::string>());
        keyColumns.push_back(cell.cellReference().column());
    }
    int keyCount = keys.size();

    // 열 번호 -> 범위 검사 종류 / 키 이름 (2행 타입)
    auto typesRow = sheet.row(2);
    std::vector<NumericColumn> numericColumns;
    std::vector<std::string> columnKeys;
    for (size_t ii = 0; ii < keyColumns.size(); ++ii)
    {
        const uint16_t column = keyColumns[ii];
        if (numericColumns.size() <= column)
        {
            numericColumns.resize(column + 1, NumericColumn::None);
            columnKeys.resize(column + 1);
        }

        std::string typeName;
        auto typeCell = typesRow.findCell(column);
        if (false == typeCell.empty() && typeCell.value().type() == OpenXLSX::XLValueType::String)
        {
            typeName = typeCell.value().getString();
        }
        numericColumns[column] = ToNumericColumn(keys[ii], typeName);
        columnKeys[column] = keys[ii];
    }

    // rowCount()는 마지막 <row>의 행 번호(1부터)다 -> 마지막 행까지 포함해야 한다 (Q2: '<'면 마지막 데이터 행이 빠졌다)
    auto rowLimit = sheet.rowCount();
    for (uint32_t ii = 3; ii <= rowLimit; ++ii)
    {
        auto row = sheet.row(ii);
        auto cellCount = row.cellCount();
        if (0 == row.cellCount())
        {
            break;
        }

        if (keyCount != cellCount)
        {
            Logger::Warning(std::format("{}시트의 {}라인의 데이터 값에 빈값이 있거나 넘칩니다.", sheet.name(), ii));
            continue;
        }

        auto firstCell = row.findCell(1);
        // 첫 칸 <c>가 아예 없으면 value()가 abort하므로 먼저 확인한다 (DataApi ScanRows와 같은 규칙)
        if (firstCell.empty() || firstCell.value().type() == OpenXLSX::XLValueType::Empty)
        {
            break;
        }

        if (firstCell.value().type() == OpenXLSX::XLValueType::String)
        {
            if (firstCell.value().getString().starts_with("#"))
            {
                continue;
            }
        }

        std::vector<std::string> values;
        for (auto& cell : row.cells())
        {
            const auto& value = cell.value();
            std::string text = CellValueToString(value);

            const auto valueType = value.type();
            if (valueType == OpenXLSX::XLValueType::Integer || valueType == OpenXLSX::XLValueType::Float || valueType == OpenXLSX::XLValueType::Error)
            {
                if (const auto raw = ReadRawNumberText(cell))
                {
                    const uint16_t column = cell.cellReference().column();
                    const NumericColumn numericColumn = column < numericColumns.size() ? numericColumns[column] : NumericColumn::None;
                    if (const auto reason = FindTooLargeNumber(value, *raw, numericColumn))
                    {
                        // 너무 큰 수는 데이터에 넣지 않는다: 값은 0, 행은 유지
                        const std::string columnKey = column < columnKeys.size() ? columnKeys[column] : std::string();
                        Logger::Error(std::format("{}시트 {}행 {}열({}) 값 '{}': {} -> 0으로 넣습니다.",
                            sheet.name(), ii, OpenXLSX::XLCellReference::columnAsString(column), columnKey, *raw, *reason));
                        text = "0";
                    }
                }
            }

            values.push_back(std::move(text));
        }

        rows.push_back(values);
    }

    Document doc;
    doc.SetArray();
    Document::AllocatorType& alloc = doc.GetAllocator();

    int groupIndex = -1;
    int idIndex = -1;

    for (int ii = 0; ii < keyCount; ++ii)
    {
        if (keys[ii] == "_group") groupIndex = ii;
        else if (keys[ii] == "_id") idIndex = ii;
    }

    for (const auto& row : rows)
    {
        Value obj(kObjectType);
        Value keyObj(kObjectType);

        if (groupIndex != -1 && !row[groupIndex].empty())
        {
            keyObj.AddMember("_group", Value(row[groupIndex].c_str(), alloc).Move(), alloc);
        }
        else
        {
            keyObj.AddMember("_group", Value("0", alloc).Move(), alloc);
        }

        if (idIndex != -1)
        {
            keyObj.AddMember("_id", Value(row[idIndex].c_str(), alloc).Move(), alloc);
        }
        else
        {
            Logger::Error(std::format("{}시트의 키값이 존재하지 않습니다 키컬럼(id)은 필수 입니다.", sheet.name()));
            break;
        }

        obj.AddMember("_key", keyObj, alloc);
        for (size_t ii = 0; ii < keys.size(); ++ii)
        {
            if (ii == groupIndex || ii == idIndex) continue;

            obj.AddMember(
                Value(keys[ii].c_str(), alloc).Move(),
                Value(row[ii].c_str(), alloc).Move(),
                alloc
            );
        }

        doc.PushBack(obj, alloc);
    }


    Writer<StringBuffer> writer(_buffer);
    doc.Accept(writer);

    return _buffer.GetString();
}

const char* JsonParser::BuildSheetIndex(const EXCEL::SheetIndex& index)
{
    _buffer.Clear();

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    for (const auto& [sheet, path] : index)
    {
        Value key(sheet.c_str(), alloc);
        Value value(path.c_str(), alloc);

        doc.AddMember(key.Move(), value.Move(), alloc);
    }

    Writer<StringBuffer> writer(_buffer);
    doc.Accept(writer);
    return _buffer.GetString();
}
