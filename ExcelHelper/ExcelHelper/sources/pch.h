// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#ifndef PCH_H
#define PCH_H

#ifdef EXCELHELPER_EXPORTS
#define O2UN_API __declspec(dllexport)
#else
#define O2UN_API __declspec(dllimport)
#endif // EXCELHELPER_EXPORTS

#include "Singleton.h"

#endif // PCH_H