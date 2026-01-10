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
    auto tempalte = GetOrLoadTemplate(TEMPLATE_STATIC_DATA);
    if (tempalte.empty())
    {
        return false;
    }

    std::string outputContent{ tempalte };
    ReplaceTag(outputContent, "#CLASSNAME#", sheet.name());
    ReplaceTag(outputContent, "#PROJECT_API#", _projectName);
    ReplaceTag(outputContent, "#VALUES#", BuildValuesSection(sheet));

    SaveFile(sheet.name(), outputContent);

    return true;
}

bool CodeGenerator::CreateDataManager(const OpenXLSX::XLWorksheet& sheet)
{
    return true;
}

std::string_view CodeGenerator::GetOrLoadTemplate(int resourceID)
{
    if (!_templateCache.contains(resourceID)) {
        // DLL 핸들(시작 주소) 획득
        HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);

        // "TEXT" 타입의 리소스 위치 찾기
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceID), L"TEXT");
        if (!hRes) return "";

        // 리소스 데이터 로드 및 크기 획득
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

std::string CodeGenerator::BuildValuesSection(const OpenXLSX::XLWorksheet& sheet) 
{
    std::string values;

    auto namesRow = sheet.row(1);
    auto typesRow = sheet.row(2);

    uint32_t cellCount = namesRow.cellCount();
    if (cellCount != typesRow.cellCount())
    {
        return values;
    }

    for (uint32_t ii = 1; ii <= cellCount; ++ii)
    {
        std::string colName = namesRow.findCell(ii).value().get<std::string>();
        std::string colType = typesRow.findCell(ii).value().get<std::string>();

        if (colName.empty() || colType.empty()) continue;
        if (Str::Equals(colType,"key")) continue;

        values += "    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"StaticData\")\n";
        values += std::format("    {} {};\n\n", MapToCppType(colType), colName);
    }

    return values;
}

std::string CodeGenerator::MapToCppType(std::string_view excelType)
{
    if (Str::Equals(excelType,"int")) return "int32";
    if (Str::Equals(excelType,"float")) return "float";
    if (Str::Equals(excelType,"string")) return "FString";
    if (Str::Equals(excelType,"bool")) return "bool";
    return "FString";
}

void CodeGenerator::SaveFile(std::string_view className, std::string_view content)
{
    try
    {
        fs::path outPath = std::format("{}/Public/StaticData/{}StaticData.h", _codePath, className);
        fs::create_directories(outPath.parent_path());

        std::ofstream outFile(outPath);
        if (outFile.is_open())
        {
            outFile << content;
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
            outFile << std::format("#include \"StaticData/{}StaticData.h\"", className);
            outFile.close();
        }
    }
    catch (const std::ios_base::failure e)
    {
        Logger::UELog(std::format("{} _ {}", e.what(), e.code().value()));
    }
}