module;
#include "OpenXLSX.hpp"
#include "rapidjson/stringbuffer.h"

export module IJsonParser;

import ICommonType;

export class JsonParser
{
public:
	JsonParser() = default;

	const char* ExcelToJson(const OpenXLSX::XLWorksheet& sheet);
	const char* BuildSheetIndex(const EXCEL::SheetIndex& index);

private:
	rapidjson::StringBuffer _buffer;
};
