// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
export module IExcelLoader;
import <string>;
import <memory>;
import <filesystem>;
import <OpenXLSX.hpp>;

import ICodeGenerator;
import IJsonParser;
import ISingleton;

namespace fs = std::filesystem;

export class ExcelLoader : public CommonHelper::Singleton<ExcelLoader>
{
public:
	void GenerateAllDataClass(std::string_view root);
	OpenXLSX::XLWorkbook LoadWorkbook(OpenXLSX::XLDocument& doc, std::string_view fileName);
	void SetCodeGeneratePath(std::string_view projectName, std::string_view path);
	const char* LoadExcel(std::string_view path, std::string_view className);
	const char* GetExcelList(std::string_view root);

private:
	template <typename Func, typename... Args>
	auto ProcessAllWorkbooks(std::string_view root, Func&& func, Args&&...args);

	template <typename Func, typename... Args>
	auto ProcessSheet(OpenXLSX::XLWorksheet sheet, Func&& func, Args&&...args);
	template <typename Func, typename... Args>
	auto ProcessSheets(OpenXLSX::XLWorkbook wb, Func&& func, Args&&...args);
	template <typename Func, typename... Args>
	auto ProcessAllSheets(std::string_view root, Func&& func, Args&&...args);

	std::vector<fs::path> LoadAllFilePath(std::string_view root);

	CodeGenerator _codeGenerator;
	JsonParser _jsonParser;
};
