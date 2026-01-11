module;
#include "pch.h"
#include "OpenXLSX.hpp"

export module ICodeGenerator;

export class CodeGenerator
{
public:
	CodeGenerator() = default;

	void GenerateHeader(const OpenXLSX::XLWorksheet& sheet);
	void SetCodeGeneratePath(std::string_view api, std::string_view path);

private:
	struct GeneratedCodeResult
	{
		std::string properties;
		std::string inits;
		std::string assigns;
	};

	std::unordered_map<int, std::string> _templateCache;
	std::string _projectName;
	std::string _codePath;

	std::string_view GetOrLoadTemplate(int resourceID);
	bool CreateStaticData(const OpenXLSX::XLWorksheet& sheet);
	bool CreateDataManager(const OpenXLSX::XLWorksheet& sheet);
	void ReplaceTag(std::string& target, std::string_view tag, std::string_view value);
	GeneratedCodeResult BuildValuesSection(const OpenXLSX::XLWorksheet& sheet);
	std::string MapToCppType(std::string_view excelType);
	void SaveFile(std::string_view className, std::string_view header, std::string_view cpp);
};