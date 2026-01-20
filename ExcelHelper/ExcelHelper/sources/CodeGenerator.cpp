// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
module;
#include "../resource.h"

module ICodeGenerator;

import <Windows.h>;
import <fstream>;
import <format>;
import <iostream>;
import <unordered_map>;
import <filesystem>;
import ICodeGenerator;
import IStringHelper;
import IContainerHelper;
import ITimeHelper;
import ILogger;

namespace fs = std::filesystem;
extern "C" IMAGE_DOS_HEADER __ImageBase;

void CodeGenerator::GenerateHeader(const OpenXLSX::XLWorksheet& sheet)
{
    if (false == CreateStaticData(sheet))
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

bool CodeGenerator::CreateStaticData(const OpenXLSX::XLWorksheet& sheet)
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

    auto [properties, inits, assigns] = BuildValuesSection(sheet);
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

        _templateCache[resourceID] = std::string(pData, size);
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

    uint32_t cellCount = namesRow.cellCount();
    if (cellCount != typesRow.cellCount())
    {
        return result;
    }

    for (uint32_t ii = 1; ii <= cellCount; ++ii)
    {
        std::string colName = namesRow.findCell(ii).value().get<std::string>();
        std::string colType = typesRow.findCell(ii).value().get<std::string>();

        if (colName.empty() || colType.empty()) continue;
        if (CommonHelper::Equals(colType,"key")) continue;

        result.properties += "\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"StaticData\")\n";
        result.properties += std::format("\t{} _{};\n\n", MapToCppType(colType), colName);
        result.inits += std::format("\tUPROPERTY()\n\t{} _{};\n", MapToCppType(colType), colName);
        result.assigns += std::format("\t_{} = initor._{};\n", colName ,colName);
    }

    return result;
}

std::string CodeGenerator::MapToCppType(std::string_view excelType)
{
    static const std::unordered_map<std::string_view, std::string> typeMap = {
        { "int",    "int32"   },
        { "float",  "float"   },
        { "string", "FString" },
        { "bool",   "bool"    }
    };

    std::string lowerType{ excelType };
    std::ranges::transform(lowerType, lowerType.begin(),::tolower);
    return CommonHelper::GetValueOrDefault(typeMap, lowerType, std::string{ "FString" });
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
