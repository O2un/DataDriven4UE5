module;
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <format>

module IJsonParser;
import <string>;
import <vector>;
import ILogger;

using namespace rapidjson;

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
    //auto typesRow = sheet.row(2);
    for (auto& cell : namesRow.cells())
    {
        keys.push_back("_" + cell.value().get<std::string>());
    }
    int keyCount = keys.size();

    auto rowLimit = sheet.rowCount();
    for (uint32_t ii = 3; ii < rowLimit; ++ii)
    {
        auto row = sheet.row(ii);
        auto cellCount = row.cellCount();
        if (0 == row.cellCount())
        {
            break;
        }

        if (keyCount != cellCount)
        {
            Logger::UELog(std::format("{}시트의 {}라인의 데이터 값에 빈값이 있거나 넘칩니다.", sheet.name(), ii));
            continue;
        }

        auto firstCell = row.findCell(1);
        if (firstCell.value().type() == OpenXLSX::XLValueType::Empty)
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
            values.push_back(cell.value().getString());
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
            Logger::UELog(std::format("{}시트의 키값이 존재하지 않습니다 키컬럼(id)은 필수 입니다.", sheet.name()));
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