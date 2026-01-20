// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
module IExcelLoader;

import <filesystem>;
import <string>;
import <vector>;

import ICommonType;
import ILogger;

namespace fs = std::filesystem;

std::vector<fs::path> ExcelLoader::LoadAllFilePath(std::string_view root)
{
	std::vector<fs::path> files;
	try
	{
		if (false == fs::exists(root) || false == fs::is_directory(root))
		{
			return files;
		}

		for (const auto& entry : fs::recursive_directory_iterator(root))
		{
			const auto& path = entry.path();
			if (false == fs::is_regular_file(path))
			{
				continue;
			}

			auto fileName = path.filename().native();
			if (fileName.empty() ||
				fileName.starts_with(L"~&") ||
				fileName.starts_with(L"#"))
			{
				continue;
			}

			auto ext = path.extension().native();
			if (L".xlsx" == ext || L".xlsm" == ext)
			{
				files.push_back(entry.path());
			}
		}
	}
	catch (const fs::filesystem_error& e)
	{
		Logger::UELog(std::format("Error : {}", e.what()));
	}

	return files;
}

template<typename Func, typename ...Args>
auto ExcelLoader::ProcessAllWorkbooks(std::string_view root, Func&& func, Args && ...args)
{
	using ReturnType = std::invoke_result_t<Func, Args..., OpenXLSX::XLDocument&, OpenXLSX::XLWorkbook>;
	constexpr bool hasReturn = !std::is_same_v<ReturnType, void>;

	std::conditional_t<hasReturn, std::vector<ReturnType>, void*> results;
	if constexpr (hasReturn) results = {};

	auto list = LoadAllFilePath(root);
	for (const auto& path : list)
	{
		OpenXLSX::XLDocument doc;
		auto wb = LoadWorkbook(doc, path.string());
		if (false == wb.valid())
		{
			continue;
		}

		if constexpr (hasReturn) 
		{
			results.push_back(std::invoke(std::forward<Func>(func), std::forward<Args>(args)..., doc, wb));
		}
		else 
		{
			std::invoke(std::forward<Func>(func), std::forward<Args>(args)...,doc, wb);
		}
	}
	if constexpr (hasReturn) return results;
}

template<typename Func, typename ...Args>
auto ExcelLoader::ProcessAllSheets(std::string_view root, Func&& func, Args && ...args)
{
	using ReturnType = std::invoke_result_t<Func, Args..., OpenXLSX::XLWorksheet>;
	constexpr bool hasReturn = !std::is_same_v<ReturnType, void>;

	std::conditional_t<hasReturn, std::vector<ReturnType>, void*> totalResults;
	if constexpr (hasReturn) totalResults = {};

	auto list = LoadAllFilePath(root);
	for (const auto& path : list)
	{
		OpenXLSX::XLDocument doc;
		auto wb = LoadWorkbook(doc, path.string());
		if (false == wb.valid())
		{
			continue;
		}

		if constexpr (hasReturn) 
		{
			auto fileResults = ProcessSheets(wb, std::forward<Func>(func), std::forward<Args>(args)...);
			totalResults.insert(totalResults.end(),
				std::make_move_iterator(fileResults.begin()),
				std::make_move_iterator(fileResults.end()));
		}
		else 
		{
			ProcessSheets(wb, std::forward<Func>(func), std::forward<Args>(args)...);
		}
	}
	if constexpr (hasReturn) return totalResults;
}

template<typename Func, typename ...Args>
auto ExcelLoader::ProcessSheets(OpenXLSX::XLWorkbook wb, Func&& func, Args&&... args)
{
	using ReturnType = std::invoke_result_t<Func, Args..., OpenXLSX::XLWorksheet>;
	constexpr bool hasReturn = !std::is_same_v<ReturnType, void>;

	if constexpr (hasReturn) 
	{
		std::vector<ReturnType> results;
		for (const auto& name : wb.sheetNames()) 
		{
			if (name.starts_with("#")) continue;
			results.push_back(ProcessSheet(wb.worksheet(name), std::forward<Func>(func), std::forward<Args>(args)...));
		}
		return results;
	}
	else 
	{
		for (const auto& name : wb.sheetNames()) 
		{
			if (name.starts_with("#")) continue;
			ProcessSheet(wb.worksheet(name), std::forward<Func>(func), std::forward<Args>(args)...);
		}
	}
}

template<typename Func, typename ...Args>
auto ExcelLoader::ProcessSheet(OpenXLSX::XLWorksheet sheet, Func&& func, Args && ...args)
{
	return std::invoke(std::forward<Func>(func), std::forward<Args>(args)..., sheet);
}

void ExcelLoader::GenerateAllDataClass(std::string_view root)
{
	ProcessAllSheets(root, &CodeGenerator::GenerateHeader, &_codeGenerator);
}

OpenXLSX::XLWorkbook ExcelLoader::LoadWorkbook(OpenXLSX::XLDocument& doc, std::string_view fileName)
{
	try
	{
		doc.open(fileName.data());
		return doc.workbook();
	}
	catch (...)
	{
		return OpenXLSX::XLWorkbook();
	}
}

void ExcelLoader::SetCodeGeneratePath(std::string_view projectName, std::string_view path)
{
	_codeGenerator.SetCodeGeneratePath(projectName, path);
}

const char* ExcelLoader::LoadExcel(std::string_view path, std::string_view className)
{
	OpenXLSX::XLDocument doc;
	auto wb = LoadWorkbook(doc, path);
	if (false == wb.valid())
	{
		Logger::UELog(std::format("엑셀 파일{}이 존재하지 않습니다.", path));
		return "";
	}

	return ProcessSheet(wb.worksheet(className.data()), &JsonParser::ExcelToJson, &_jsonParser);
}

const char* ExcelLoader::GetExcelList(std::string_view root)
{
	EXCEL::SheetIndex index;

	ProcessAllWorkbooks(root, [&](OpenXLSX::XLDocument& doc, OpenXLSX::XLWorkbook wb) {
		std::string path = doc.path();
		auto names = wb.sheetNames();
		for (const auto& name : names)
		{
			if (name.starts_with("#"))
			{
				continue;
			}

			if (index.contains(name.data()))
			{
				Logger::UELog(std::format("엑셀 경로{} 중복된 시트이름{} 입니다 수정해주세요", doc.path(), name));
			}
			else
			{
				index.emplace(std::string(name), doc.path());
			}
		}
		});

	return _jsonParser.BuildSheetIndex(index);
}
