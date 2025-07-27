#pragma once
#include <string>
#include <vector>
#include <memory>       // 引入 memory 以使用 std::unique_ptr
#include "Message.hpp"  // 依赖重构后的 Message.hpp

namespace mylog
{
    // 不再需要依赖全局配置文件来管理缓冲区大小，std::vector 会自动管理内存
    // extern mylog::Util::JsonData* g_conf_data;

    class Buffer
    {
    public:
        // 构造函数，可以预分配一定容量以提高性能
        Buffer() {
            // 预分配空间可以减少 vector 在 Push 过程中的多次重新分配开销。
            // 这是一个可以根据实际场景调整的优化点。
            buffer_.reserve(1024);
        }

        // 接收一个 LogMessage 对象的 unique_ptr，并将其移入缓冲区
        // 这是生产者（AsyncLogger）向缓冲区添加日志的唯一入口。
        void Push(std::unique_ptr<LogMessage>&& msg)
        {
            buffer_.emplace_back(std::move(msg));
        }

        // 判断缓冲区是否为空
        bool IsEmpty() const
        {
            return buffer_.empty();
        }

        // 获取缓冲区中的日志条目数量
        size_t Size() const
        {
            return buffer_.size();
        }

        // 与另一个缓冲区交换内容。
        // 这是实现“双缓冲无锁交换”的核心操作，效率极高。
        void Swap(Buffer& other)
        {
            buffer_.swap(other.buffer_);
        }

        // 清空缓冲区，释放所有 LogMessage 对象。
        // 当消费者处理完一个缓冲区后调用。
        void Reset()
        {
            buffer_.clear();
        }

        // 提供对内部元素的访问（为消费者线程提供）
        // 返回 const 引用，防止外部修改指针
        const std::unique_ptr<LogMessage>& at(size_t index) const
        {
            return buffer_.at(index);
        }

    private:
        // 核心数据结构：一个存储 LogMessage 智能指针的 vector。
        std::vector<std::unique_ptr<LogMessage>> buffer_;

    };

}