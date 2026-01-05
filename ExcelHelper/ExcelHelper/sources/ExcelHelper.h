#pragma once
#include "pch.h"

extern "C" 
{
	typedef void(*logCallback)(const char*);
	O2UN_API void SetLogHandler(logCallback c);
	O2UN_API void Test(const char* root);
}


