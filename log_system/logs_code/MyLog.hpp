#pragma once
#include "Manager.hpp"
namespace mylog {
// 用户获取日志器
AsyncLogger::ptr GetLogger(const std::string &name) {
    return LoggerManager::GetInstance().GetLogger(name);
}
// 用户获取默认日志器
AsyncLogger::ptr DefaultLogger() { return LoggerManager::GetInstance().DefaultLogger(); }

// 简化用户使用，宏函数默认填上文件吗+行号
#define Debug(fmt, ...) Debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Info(fmt, ...) Info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Warn(fmt, ...) Warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Error(fmt, ...) Error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Fatal(fmt, ...) Fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 无需获取日志器，默认标准输出
#define LOGDEBUGDEFAULT(fmt, ...) mylog::DefaultLogger()->Debug(fmt, ##__VA_ARGS__)
#define LOGINFODEFAULT(fmt, ...) mylog::DefaultLogger()->Info(fmt, ##__VA_ARGS__)
#define LOGWARNDEFAULT(fmt, ...) mylog::DefaultLogger()->Warn(fmt, ##__VA_ARGS__)
#define LOGERRORDEFAULT(fmt, ...) mylog::DefaultLogger()->Error(fmt, ##__VA_ARGS__)
#define LOGFATALDEFAULT(fmt, ...) mylog::DefaultLogger()->Fatal(fmt, ##__VA_ARGS__)
}  // namespace mylog