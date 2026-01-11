// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "ExcelHelper.h"
import IExcelLoader;
import ILogger;

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
