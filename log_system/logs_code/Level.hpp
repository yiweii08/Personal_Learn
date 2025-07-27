#pragma once
#include <string>

namespace mylog {
class LogLevel {
   public:
    enum class value { DEBUG, INFO, WARN, ERROR, FATAL};

    // 提供日志等级的字符串转换接口
    static const char* ToString(value level) {
        switch (level) {
            case value::DEBUG:
                return "DEBUG";
            case value::INFO:
                return "INFO";
            case value::WARN:
                return "WARN";
            case value::ERROR:
                return "ERROR";
            case value::FATAL:
                return "FATAL";
            default:
                return "UNKNOW";
        }
        return "UNKNOW";
    }
};
}  // namespace mylog
