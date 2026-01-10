#include "pch.h"
#include "ExcelHelper.h"
#include "Logger.h"

import IExcelLoader;

void SetLogHandler(logCallback c)
{
	Logger::Instance().SetLogHandler(c);
}

void SetCodeGenerateInfo(const char* projectName, const char* path)
{
	ExcelLoader::Instance().SetCodeGeneratePath(projectName, path);
}

void GenerateAllDataClass(const char* root)
{
	ExcelLoader::Instance().GenerateAllDataClass(root);
}
