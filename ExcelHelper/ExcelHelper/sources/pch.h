#ifndef PCH_H
#define PCH_H

#ifdef EXCELHELPER_EXPORTS
#define O2UN_API __declspec(dllexport)
#else
#define O2UN_API __declspec(dllimport)
#endif // EXCELHELPER_EXPORTS

#include "Singleton.h"

#endif // PCH_H