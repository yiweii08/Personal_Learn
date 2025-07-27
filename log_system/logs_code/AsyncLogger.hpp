#pragma once

#include <atomic>
#include <cstdarg>
#include <memory>
#include <string>
#include <vector>

#include "Level.hpp"
#include "Message.hpp"
#include "LogFlush.hpp"
#include "LogPipeline.hpp" // 依赖新的 Pipeline
#include "ThreadPoll.hpp"   // 依赖外部线程池
#include "backlog/CliBackupLog.hpp" // 依赖备份函数

// 远程备份功能依赖的外部全局线程池指针
// extern ThreadPool *tp;

namespace mylog
{
    class AsyncLogger
    {
    public:
        using ptr = std::shared_ptr<AsyncLogger>;

        // 构造函数：只接收日志器名和落地器列表
        AsyncLogger(const std::string& logger_name, const std::vector<LogFlush::ptr>& flushs)
            : logger_name_(logger_name),
              pipeline_(std::make_shared<LogPipeline>(flushs)) // 创建并持有 Pipeline
        {}

        virtual ~AsyncLogger() = default;

        std::string Name() { return logger_name_; }

        // 为了代码简洁和统一，我们将可变参数的处理逻辑集中到一个私有方法中
        void Log(LogLevel::value level, const char* file, size_t line, const char* format, ...)
        {
            va_list va;
            va_start(va, format);
            Handle(level, file, line, format, va);
            va_end(va);
        }

    private:
        // 私有辅助函数，处理可变参数并封装为 LogMessage
        void Handle(LogLevel::value level, const char* file, size_t line, const char* format, va_list va)
        {
            // 使用 vsnprintf 避免内存泄漏和不可移植问题
            va_list va_copy;
            va_copy(va_copy, va);
            int len = vsnprintf(nullptr, 0, format, va_copy);
            va_end(va_copy);

            if (len < 0) return;
            
            std::string payload(len, '\0');
            vsnprintf(&payload[0], len + 1, format, va);
            
            // 创建 LogMessage 对象
            auto msg = std::make_unique<LogMessage>(level, file, line, logger_name_, std::move(payload));
            
            // 分配序列号
            msg->sequence_id_ = g_sequence_id_allocator.fetch_add(1, std::memory_order_relaxed);
            
            // ----------- 远程备份逻辑（保持独立） -----------
            // 检查日志级别，决定是否触发独立的备份任务
            // if (tp && msg->level_ >= LogLevel::value::ERROR)
            // {
            //     try {
            //         // 格式化日志以供发送
            //         std::string data_for_backup = msg->format();
            //         tp->enqueue(start_backup, data_for_backup);
            //     } catch(const std::runtime_error&) {
            //         // 外部线程池已关闭，忽略
            //     }
            // }
            // ---------------------------------------------
            
            // 将主日志流的消息推入 Pipeline
            pipeline_->Push(std::move(msg));
        }

    public:
        // 为用户提供简洁的接口，内部调用 Log 方法
        void Debug(const std::string &file, size_t line, const std::string format, ...) {
            va_list va;
            va_start(va, format);
            Handle(LogLevel::value::DEBUG, file.c_str(), line, format.c_str(), va);
            va_end(va);
        }
        void Info(const std::string &file, size_t line, const std::string format, ...) {
            va_list va;
            va_start(va, format);
            Handle(LogLevel::value::INFO, file.c_str(), line, format.c_str(), va);
            va_end(va);
        }
        void Warn(const std::string &file, size_t line, const std::string format, ...) {
            va_list va;
            va_start(va, format);
            Handle(LogLevel::value::WARN, file.c_str(), line, format.c_str(), va);
            va_end(va);
        }
        void Error(const std::string &file, size_t line, const std::string format, ...) {
            va_list va;
            va_start(va, format);
            Handle(LogLevel::value::ERROR, file.c_str(), line, format.c_str(), va);
            va_end(va);
        }
        void Fatal(const std::string &file, size_t line, const std::string format, ...) {
            va_list va;
            va_start(va, format);
            Handle(LogLevel::value::FATAL, file.c_str(), line, format.c_str(), va);
            va_end(va);
        }


    private:
        std::string logger_name_;
        std::shared_ptr<LogPipeline> pipeline_;
    };

    // LoggerBuilder 的代码无需修改，它与这些底层实现无关
    class LoggerBuilder
    {
    public:
        using ptr = std::shared_ptr<LoggerBuilder>;
        
        void BuildLoggerName(const std::string &name) { logger_name_ = name; }
        
        template <typename FlushType, typename... Args>
        void BuildLoggerFlush(Args&&... args)
        {
            flushs_.emplace_back(LogFlushFactory::CreateLog<FlushType>(std::forward<Args>(args)...));
        }
        
        AsyncLogger::ptr Build()
        {
            if (logger_name_.empty()) {
                logger_name_ = "default_logger";
            }
            if (flushs_.empty()) {
                flushs_.emplace_back(std::make_shared<StdoutFlush>());
            }
            return std::make_shared<AsyncLogger>(logger_name_, flushs_);
        }

    protected:
        std::string logger_name_ = "async_logger";
        std::vector<mylog::LogFlush::ptr> flushs_;
    };

} // namespace mylog