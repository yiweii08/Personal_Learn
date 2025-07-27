#pragma once

#include <memory>
#include <thread>
#include <atomic>   // 引入 atomic 头文件
#include <sstream>  // 引入 sstream 头文件以使用 stringstream

#include "Level.hpp"
#include "Util.hpp"

namespace mylog
{
    // ============================= 新增部分 =============================
    // 定义一个全局、原子的序列号分配器。
    // 每条日志在创建时都会从这里获取一个独一无二且严格递增的ID。
    // 这是保证日志最终能按序写入的关键。
    // 使用 memory_order_relaxed 是因为我们只关心原子性和唯一递增，不依赖它进行线程间的内存同步。
    static std::atomic<uint64_t> g_sequence_id_allocator(0);
    // =================================================================

    struct LogMessage
    {
        using ptr = std::shared_ptr<LogMessage>;
        LogMessage() = default;
        LogMessage(LogLevel::value level, const std::string& file, size_t line,
                   const std::string& name, const std::string& payload)
            : level_(level),
              file_name_(file),
              line_(line),
              name_(name),
              payload_(payload),
              ctime_(Util::Date::Now()),
              tid_(std::this_thread::get_id()),
              // 注意：sequence_id_ 在这里不初始化，它将在被创建时由外部赋值
              sequence_id_(0)
        {}

        // format 方法保持不变，它负责将日志消息的所有信息格式化为最终的字符串。
        // 在新的架构中，这个方法将由消费者线程（Formatter Thread）调用。
        std::string format()
        {
            std::stringstream ret;
            // 获取当前时间
            struct tm t;
            localtime_r(&ctime_, &t);
            char buf[128];
            strftime(buf, sizeof(buf), "%H:%M:%S", &t);
            // 格式: [时间][线程ID][日志级别][日志器名][文件名:行号] <Tab> 日志正文 <换行>
            std::string tmp1 = '[' + std::string(buf) + "][";
            std::string tmp2 = "][" + std::string(LogLevel::ToString(level_)) + "][" + name_ + "][" + file_name_ + ":" + std::to_string(line_) + "]\t" + payload_ + "\n";
            ret << tmp1 << tid_ << tmp2;
            return ret.str();
        }

        time_t ctime_;          // 时间戳
        std::thread::id tid_;   // 线程id
        LogLevel::value level_; // 日志等级
        size_t line_;           // 行号
        std::string file_name_; // 文件名
        std::string name_;      // 日志器名
        std::string payload_;   // 日志正文

        // ============================= 新增部分 =============================
        // 全局唯一序列号，由 g_sequence_id_allocator 分配。
        // 用于在并行处理后，由 I/O 线程进行重新排序，确保日志写入的顺序性。
        uint64_t sequence_id_;
        // =================================================================
    };
} // namespace mylog