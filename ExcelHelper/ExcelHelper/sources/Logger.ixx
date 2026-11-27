export module ILogger;

import ISingleton;
import <cstdint>;
import <string>;
import <string_view>;

// 등급 값은 ExcelHelper.h의 O2UN_LOG_* 와 같아야 한다 (C ABI)
export enum class LogLevel : std::int32_t
{
    Info = 0,
    Warning = 1,
    Error = 2,
    Fatal = 3,
};

export typedef void(*LogHandler)(std::int32_t level, const char* message);
export class Logger : public CommonHelper::Singleton<Logger>
{
public:
    static void Info(std::string_view msg) { Instance().Log(LogLevel::Info, msg); }
    static void Warning(std::string_view msg) { Instance().Log(LogLevel::Warning, msg); }
    static void Error(std::string_view msg) { Instance().Log(LogLevel::Error, msg); }
    // 작업 전체를 중단해야 하는 오류. 에디터를 종료시키지는 않는다 (UE 쪽에서 Error로 표시)
    static void Fatal(std::string_view msg) { Instance().Log(LogLevel::Fatal, msg); }

    void SetLogHandler(LogHandler handle)
    {
        _logHandler = handle;
    }

    // 범위 안에서 로그를 내보내지 않는다 (O2un_* 함수가 검증용으로 로더를 돌릴 때 기존 시트 경고가 반복되지 않게).
    // DLL 호출은 호출자가 직렬화한다는 계약(ExcelHelper.h)이므로 카운터는 원자적이지 않아도 된다
    class MuteScope
    {
    public:
        MuteScope() { ++Instance()._muteDepth; }
        ~MuteScope() { --Instance()._muteDepth; }
        MuteScope(const MuteScope&) = delete;
        MuteScope& operator=(const MuteScope&) = delete;
    };
private:
    void Log(LogLevel level, std::string_view msg)
    {
        if (!_logHandler || 0 < _muteDepth) return;
        // string_view는 널 종료가 보장되지 않으므로 복사해서 넘긴다
        const std::string message(msg);
        _logHandler(static_cast<std::int32_t>(level), message.c_str());
    }
    LogHandler _logHandler = nullptr;
    int _muteDepth = 0;
};
