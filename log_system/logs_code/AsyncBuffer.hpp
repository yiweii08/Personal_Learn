#pragma once
#include <string>
#include <vector>
#include <memory>       
#include "Message.hpp" 

namespace mylog
{
    // extern mylog::Util::JsonData* g_conf_data;

    class Buffer
    {
    public:
        Buffer() {
            // 预分配空间
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

        // 提供对内部元素的访问
        // 返回 const 引用，防止外部修改指针
        const std::unique_ptr<LogMessage>& at(size_t index) const
        {
            return buffer_.at(index);
        }

    private:
        std::vector<std::unique_ptr<LogMessage>> buffer_;

    };

}
