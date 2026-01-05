module;
#include "pch.h"
#include <string>
#include <memory>
#include <filesystem>
#include <OpenXLSX.hpp>

export module IExcelLoader;

import ICodeGenerator;

namespace fs = std::filesystem;

export class ExcelLoader : public Singleton<ExcelLoader>
{
public:
	void GenerateAllDataClass(std::string_view root);
	OpenXLSX::XLWorkbook LoadWorkbook(OpenXLSX::XLDocument& doc, std::string_view fileName);

private:
	void ProcessAllFiles(std::string_view root, auto&& func, auto&&...args);
	std::vector<fs::path> LoadAllFilePath(std::string_view root);

	CodeGenerator _codeGenerator;
};
