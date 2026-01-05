module;
#include "pch.h"
#include "OpenXLSX.hpp"

export module ICodeGenerator;

export class CodeGenerator
{
public:
	void GenerateHeader(const OpenXLSX::XLWorksheet& sheet);
};