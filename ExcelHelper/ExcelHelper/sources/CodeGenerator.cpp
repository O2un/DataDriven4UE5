module;
#include "pch.h"
#include "Logger.h"

import ICodeGenerator;

void CodeGenerator::GenerateHeader(const OpenXLSX::XLWorksheet& sheet)
{
	Logger::UELog(sheet.name());
}