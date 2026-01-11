// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "pch.h"
#include <vector>
#include <string>

typedef void(*LogHandler)(const char*);
class Logger : public Singleton<Logger>
{
public:
    static void UELog(std::string_view msg)
    {
        Instance().Log(msg);
    }
    void SetLogHandler(LogHandler handle)
    {
        _logHandler = handle;
    }
private:
    void Log(std::string_view msg)
    {
        if (_logHandler) _logHandler(msg.data());
    }
    LogHandler _logHandler = nullptr;
};