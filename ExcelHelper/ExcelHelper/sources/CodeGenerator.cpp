module;
#include "pch.h"
#include "Logger.h"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <unordered_map>
#include "../resource.h"

import ICodeGenerator;
import StringHelper;
import ContainerHelper;

namespace fs = std::filesystem;
extern "C" IMAGE_DOS_HEADER __ImageBase;

void CodeGenerator::GenerateHeader(const OpenXLSX::XLWorksheet& sheet)
{
    if (false == CreateStaticData(sheet))
    {
        Logger::UELog(std::format("{}시트의 StaticData 파일 생성 실패", sheet.name()));
    }
    if (false == CreateDataManager(sheet))
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

    std::string headerContent{ header };
    {
        ReplaceTag(headerContent, "#CLASSNAME#", sheet.name());
        ReplaceTag(headerContent, "#PROJECT_API#", _projectName);

        ReplaceTag(headerContent, "#VALUES#", properties);
        ReplaceTag(headerContent, "#INITOR#", inits);
    }

    std::string cppContent{ cpp };
    {
        ReplaceTag(cppContent, "#CLASSNAME#", sheet.name());
        ReplaceTag(cppContent, "#ASSIGN#", assigns);
    }

    SaveFile(sheet.name(), headerContent, cppContent);

    return true;
}

bool CodeGenerator::CreateDataManager(const OpenXLSX::XLWorksheet& sheet)
{
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
        result.inits += std::format("\t{} _{};\n", MapToCppType(colType), colName);
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

void CodeGenerator::SaveFile(std::string_view className, std::string_view header, std::string_view cpp)
{
    try
    {
        fs::path outPath = std::format("{}/Public/StaticData/{}StaticData.h", _codePath, className);
        fs::create_directories(outPath.parent_path());

        std::ofstream outFile(outPath);
        if (outFile.is_open())
        {
            outFile << header;
            outFile.close();
        }
    }
    catch (const std::ios_base::failure e)
    {
        Logger::UELog(std::format("{} _ {}", e.what(), e.code().value()));
    }

    try
    {
        fs::path outPath = std::format("{}/Private/StaticData/{}StaticData.cpp", _codePath, className);
        fs::create_directories(outPath.parent_path());

        std::ofstream outFile(outPath);
        if (outFile.is_open())
        {
            outFile << cpp;
            outFile.close();
        }
    }
    catch (const std::ios_base::failure e)
    {
        Logger::UELog(std::format("{} _ {}", e.what(), e.code().value()));
    }
}