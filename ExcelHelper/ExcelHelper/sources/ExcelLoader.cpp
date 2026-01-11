// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
module;
#include "pch.h"
#include <filesystem>
#include <string>
#include <vector>

#include "Logger.h"

module IExcelLoader;

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

void ExcelLoader::ProcessAllFiles(std::string_view root, auto&& func, auto&&...args)
{
	auto list = LoadAllFilePath(root);
	for (const auto& path : list)
	{
		OpenXLSX::XLDocument doc;
		auto wb = LoadWorkbook(doc, path.string());
		if (false == wb.valid())
		{
			continue;
		}

		auto names = wb.sheetNames();
		for (std::string_view name : names)
		{
			if (name.starts_with("#"))
			{
				continue;
			}

			std::invoke(std::forward<decltype(func)>(func),
						std::forward<decltype(args)>(args)...,
						wb.worksheet(name.data()));
		}
	}
}

void ExcelLoader::GenerateAllDataClass(std::string_view root)
{
	ProcessAllFiles(root, &CodeGenerator::GenerateHeader, &_codeGenerator);
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
