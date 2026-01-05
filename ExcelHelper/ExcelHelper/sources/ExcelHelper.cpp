#include "pch.h"
#include "ExcelHelper.h"
#include "Logger.h"

import IExcelLoader;

void SetLogHandler(logCallback c)
{
	Logger::Instance().SetLogHandler(c);
}

void Test(const char* root)
{
	ExcelLoader::Instance().GenerateAllDataClass(root);
}
