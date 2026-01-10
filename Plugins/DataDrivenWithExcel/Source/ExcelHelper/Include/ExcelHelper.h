#pragma once
#include "pch.h"

extern "C" 
{
	typedef void(*logCallback)(const char*);
	O2UN_API void SetLogHandler(logCallback c);
	O2UN_API void SetCodeGenerateInfo(const char* projectName, const char* root);
	O2UN_API void GenerateAllDataClass(const char* root);
}


