// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
export module IExcelLoader;
import <string>;
import <memory>;
import <filesystem>;
import <OpenXLSX.hpp>;

import ICodeGenerator;
import ISingleton;

namespace fs = std::filesystem;

export class ExcelLoader : public CommonHelper::Singleton<ExcelLoader>
{
public:
	void GenerateAllDataClass(std::string_view root);
	OpenXLSX::XLWorkbook LoadWorkbook(OpenXLSX::XLDocument& doc, std::string_view fileName);
	void SetCodeGeneratePath(std::string_view projectName, std::string_view path);

private:
	void ProcessAllFiles(std::string_view root, auto&& func, auto&&...args);
	std::vector<fs::path> LoadAllFilePath(std::string_view root);

	CodeGenerator _codeGenerator;
};
