export module ILogger;

import ISingleton;
import <string_view>;

export typedef void(*LogHandler)(const char*);
export class Logger : public CommonHelper::Singleton<Logger>
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