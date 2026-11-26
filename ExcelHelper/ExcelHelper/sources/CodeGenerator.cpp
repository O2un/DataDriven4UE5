// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
module;
#include "../resource.h"
#include "OpenXLSX.hpp"

module ICodeGenerator;

import <Windows.h>;
import <fstream>;
import <format>;
import <iostream>;
import <unordered_map>;
import <filesystem>;
import <algorithm>;
import <array>;
import <optional>;
import ICodeGenerator;
import IStringHelper;
import ITimeHelper;
import ILogger;

namespace fs = std::filesystem;
extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
    enum class HeaderCellKind
    {
        Empty,      // 셀 없음, 빈 값, 빈 문자열
        Text,       // 문자열
        NotText,    // 숫자·불리언 등 (이름/타입 칸으로 쓸 수 없음)
    };

    struct HeaderCell
    {
        HeaderCellKind kind = HeaderCellKind::Empty;
        std::string text;
    };

    // 1·2행(이름/타입) 셀을 읽는다. get<std::string>()은 숫자 셀에서 예외를 던지므로 타입을 먼저 본다
    HeaderCell ReadHeaderCell(OpenXLSX::XLRow& row, uint16_t columnNumber)
    {
        OpenXLSX::XLCell cell = row.findCell(columnNumber);
        if (!cell)
        {
            return {};
        }

        const auto& value = cell.value();
        if (value.type() == OpenXLSX::XLValueType::Empty)
        {
            return {};
        }

        // 서식만 있는 셀(<c s="1" t="n"/>처럼 값 없는 셀)은 타입이 Empty가 아닐 수 있다 -> 문자열이 비면 빈 칸으로 본다
        std::string text = value.getString();
        if (text.empty())
        {
            return {};
        }
        if (value.type() == OpenXLSX::XLValueType::String)
        {
            return { HeaderCellKind::Text, std::move(text) };
        }
        return { HeaderCellKind::NotText, std::move(text) };
    }
}

void CodeGenerator::GenerateHeader(const OpenXLSX::XLWorksheet& sheet)
{
    // 시트 하나의 예외(OpenXLSX 값 타입 오류, 파일 시스템 오류 등)가 다른 시트 생성이나 C 경계까지 번지지 않게 여기서 막는다
    std::string sheetName;
    try
    {
        sheetName = sheet.name();
        GenerateHeaderUnchecked(sheet);
    }
    catch (const std::exception& e)
    {
        Logger::UELog(std::format("{}시트 코드 생성 중 예외로 이 시트를 건너뜁니다: {}", sheetName, CommonHelper::ToUtf8FromSystem(e.what())));
    }
    catch (...)
    {
        Logger::UELog(std::format("{}시트 코드 생성 중 알 수 없는 예외로 이 시트를 건너뜁니다", sheetName));
    }
}

void CodeGenerator::GenerateHeaderUnchecked(const OpenXLSX::XLWorksheet& sheet)
{
    GeneratedCodeResult values = BuildValuesSection(sheet);
    if (false == values.errors.empty())
    {
        // 오류가 있는 시트는 StaticData/DataAsset 파일을 모두 쓰지 않는다 (기존 생성 파일은 그대로 둔다)
        for (const auto& error : values.errors)
        {
            Logger::UELog(error);
        }
        Logger::UELog(std::format("{}시트 코드 생성 중단: 오류 {}건 (기존 생성 파일은 유지)", sheet.name(), values.errors.size()));
        return;
    }

    if (false == CreateStaticData(sheet, values))
    {
        Logger::UELog(std::format("{}시트의 StaticData 파일 생성 실패", sheet.name()));
    }
    if (false == CreateDataAsset(sheet))
    {
        Logger::UELog(std::format("{}시트의 DataManager 파일 생성 실패", sheet.name()));
    }
}

void CodeGenerator::SetCodeGeneratePath(std::string_view projectName, std::string_view path)
{
    _projectName = projectName;
    _codePath = path;
}

bool CodeGenerator::CreateStaticData(const OpenXLSX::XLWorksheet& sheet, const GeneratedCodeResult& values)
{
    auto header = GetOrLoadTemplate(TEMPLATE_STATIC_DATA);
    if (header.empty())
    {
        return false;
    }

    auto cpp = GetOrLoadTemplate(TEMPLATE_STATIC_DATA_CPP);
    if (cpp.empty())
    {
        return false;
    }

    const auto& [properties, inits, assigns, errors] = values;
    std::string currentYear = CommonHelper::GetCurrentYear();
    std::string className = sheet.name();
    std::string headerContent{ header };
    {
        ReplaceTag(headerContent, "#COPYRIGHT#", std::format("// SPDX-License-Identifier: AGPL-3.0-or-later\n// Copyright (c) {} [Jaewon Cho]. All rights reserved.", currentYear));
        ReplaceTag(headerContent, "#CLASSNAME#", className);
        ReplaceTag(headerContent, "#PROJECT_API#", _projectName);

        ReplaceTag(headerContent, "#VALUES#", properties);
        ReplaceTag(headerContent, "#INITOR#", inits);
    }

    std::string cppContent{ cpp };
    {
        ReplaceTag(cppContent, "#COPYRIGHT#", std::format("// SPDX-License-Identifier: AGPL-3.0-or-later\n// Copyright (c) {} [Jaewon Cho]. All rights reserved.", currentYear));
        ReplaceTag(cppContent, "#CLASSNAME#", className);
        ReplaceTag(cppContent, "#ASSIGN#", assigns);
    }

    SaveFile(std::format("StaticData/{}StaticData", className), headerContent, cppContent);

    return true;
}

bool CodeGenerator::CreateDataAsset(const OpenXLSX::XLWorksheet& sheet)
{
    auto header = GetOrLoadTemplate(TEMPLATE_DATAASSET);
    if (header.empty())
    {
        return false;
    }

    auto cpp = GetOrLoadTemplate(TEMPLATE_DATAASSET_CPP);
    if (cpp.empty())
    {
        return false;
    }

    std::string className = sheet.name();
    std::string currentYear = CommonHelper::GetCurrentYear();
    std::string headerContent{ header };
    {
        ReplaceTag(headerContent, "#COPYRIGHT#", std::format("// SPDX-License-Identifier: AGPL-3.0-or-later\n// Copyright (c) {} [Jaewon Cho]. All rights reserved.", currentYear));
        ReplaceTag(headerContent, "#CLASSNAME#", className);
    }

    std::string cppContent{ cpp };
    {
        ReplaceTag(cppContent, "#COPYRIGHT#", std::format("// SPDX-License-Identifier: AGPL-3.0-or-later\n// Copyright (c) {} [Jaewon Cho]. All rights reserved.", currentYear));
        ReplaceTag(cppContent, "#CLASSNAME#", className);
    }

    SaveFile(std::format("DataAsset/{}DataAsset", className), headerContent, cppContent);
    return true;
}

std::string_view CodeGenerator::GetOrLoadTemplate(int resourceID)
{
    if (!_templateCache.contains(resourceID)) {
        HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceID), L"TEXT");
        if (!hRes) return "";

        HGLOBAL hData = LoadResource(hModule, hRes);
        DWORD size = SizeofResource(hModule, hRes);
        const char* pData = static_cast<const char*>(LockResource(hData));

        if (!pData) return "";

        // 템플릿은 체크아웃 설정에 따라 CRLF일 수 있다. 출력은 텍스트 모드(\n -> \r\n)로 쓰므로
        // 여기서 \r을 제거해야 \r\r\n 이 생기지 않는다.
        std::string text(pData, size);
        std::erase(text, '\r');
        _templateCache[resourceID] = std::move(text);
    }
    return _templateCache[resourceID];
}

void CodeGenerator::ReplaceTag(std::string& target, std::string_view tag, std::string_view value)
{
    size_t pos = 0;
    while ((pos = target.find(tag, pos)) != std::string::npos) 
    {
        target.replace(pos, tag.length(), value);
        pos += value.length();
    }
}

CodeGenerator::GeneratedCodeResult CodeGenerator::BuildValuesSection(const OpenXLSX::XLWorksheet& sheet)
{
    CodeGenerator::GeneratedCodeResult result;

    auto namesRow = sheet.row(1);
    auto typesRow = sheet.row(2);

    // cellCount()는 서식만 있는 빈 셀도 세므로 칸 수 비교 대신 열마다 이름/타입 짝을 확인한다
    const uint32_t columnLimit = std::max<uint32_t>(namesRow.cellCount(), typesRow.cellCount());
    for (uint32_t ii = 1; ii <= columnLimit; ++ii)
    {
        const uint16_t columnNumber = static_cast<uint16_t>(ii);
        const HeaderCell nameCell = ReadHeaderCell(namesRow, columnNumber);
        const HeaderCell typeCell = ReadHeaderCell(typesRow, columnNumber);
        const std::string columnLetter = OpenXLSX::XLCellReference::columnAsString(columnNumber);

        if (nameCell.kind == HeaderCellKind::Empty && typeCell.kind == HeaderCellKind::Empty)
        {
            continue;   // 비어 있거나 서식만 있는 열
        }
        if (nameCell.kind == HeaderCellKind::NotText)
        {
            result.errors.push_back(std::format("{}시트 {}열: 1행 이름 '{}'이(가) 문자열이 아닙니다", sheet.name(), columnLetter, nameCell.text));
            continue;
        }
        if (typeCell.kind == HeaderCellKind::NotText)
        {
            result.errors.push_back(std::format("{}시트 {}열({}): 2행 타입 '{}'이(가) 문자열이 아닙니다", sheet.name(), columnLetter, nameCell.text, typeCell.text));
            continue;
        }
        if (nameCell.kind == HeaderCellKind::Empty)
        {
            result.errors.push_back(std::format("{}시트 {}열: 2행 타입 '{}'은(는) 있는데 1행 이름이 비어 있습니다", sheet.name(), columnLetter, typeCell.text));
            continue;
        }
        if (typeCell.kind == HeaderCellKind::Empty)
        {
            result.errors.push_back(std::format("{}시트 {}열({}): 1행 이름은 있는데 2행 타입이 비어 있습니다", sheet.name(), columnLetter, nameCell.text));
            continue;
        }

        const std::string& colName = nameCell.text;
        const std::string& colType = typeCell.text;

        auto cppType = MapToCppType(colType);
        if (false == cppType.has_value())
        {
            std::string error = std::format("{}시트 {}열: 지원하지 않는 타입 '{}' (지원: {})", sheet.name(), colName, colType, BuildSupportedTypeList());
            static constexpr std::array<std::string_view, 5> nonBlueprintIntTypes{ "uint", "uint32", "uint64", "int8", "int16" };
            if (std::ranges::any_of(nonBlueprintIntTypes, [&](std::string_view t) { return CommonHelper::Equals(colType, t); }))
            {
                error += ". Blueprint에 노출할 수 없는 정수 타입입니다. int를 사용하세요";
            }
            result.errors.push_back(std::move(error));
            continue;
        }

        if (cppType->empty()) continue;   // key: 멤버를 만들지 않는다 (_key로 간다)

        result.properties += "\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"StaticData\")\n";
        result.properties += std::format("\t{} _{}{{}};\n\n", *cppType, colName);
        result.inits += std::format("\tUPROPERTY()\n\t{} _{}{{}};\n", *cppType, colName);
        result.assigns += std::format("\t_{} = initor._{};\n", colName ,colName);
    }

    return result;
}

std::optional<std::string_view> CodeGenerator::MapToCppType(std::string_view excelType)
{
    for (const auto& info : CodeGen::SupportedColumnTypes)
    {
        if (CommonHelper::Equals(excelType, info.excelType))
        {
            return info.cppType;
        }
    }
    return std::nullopt;
}

std::string CodeGenerator::BuildSupportedTypeList()
{
    std::string list;
    for (const auto& info : CodeGen::SupportedColumnTypes)
    {
        if (false == list.empty()) list += ", ";
        list += info.excelType;
    }
    return list;
}

void CodeGenerator::SaveFile(std::string_view detailPath, std::string_view header, std::string_view cpp)
{
    SaveFile(std::format("{}/Private/{}.h", _codePath, detailPath), header);
    SaveFile(std::format("{}/Public/{}.cpp", _codePath, detailPath), cpp);
}

void CodeGenerator::SaveFile(std::string_view path, std::string_view content)
{
    try
    {
        fs::path outPath{ path };
        fs::create_directories(outPath.parent_path());
        std::ofstream outFile(outPath);
        if (outFile.is_open())
        {
            const unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
            outFile.write(reinterpret_cast<const char*>(BOM), 3);
            outFile.write(content.data(), content.size());
            outFile.close();
        }
    }
    catch (const std::ios_base::failure e)
    {
        Logger::UELog(std::format("{} _ {}", e.what(), e.code().value()));
    }
}
