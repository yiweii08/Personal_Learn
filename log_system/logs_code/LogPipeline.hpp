#pragma once

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <functional>
#include <queue>
#include <memory>
#include <chrono>

#include "AsyncBuffer.hpp"
#include "LogFlush.hpp"
#include "Util.hpp"
#include"ThreadPoll.hpp"
#include "backlog/CliBackupLog.hpp"
extern mylog::Util::JsonData* g_conf_data;

namespace mylog {

struct LogBatchTask {
    std::unique_ptr<Buffer> buffer;
    
    LogBatchTask(std::unique_ptr<Buffer> buf) : buffer(std::move(buf)) {}
    LogBatchTask(const LogBatchTask&) = delete;
    LogBatchTask& operator=(const LogBatchTask&) = delete;
    LogBatchTask(LogBatchTask&&) = default;
    LogBatchTask& operator=(LogBatchTask&&) = default;
};

class LogPipeline {
public:
    LogPipeline(const std::vector<LogFlush::ptr>& flushers)
        : flushers_(flushers),
          stop_flag_(false),
          producer_buffer_(std::make_unique<Buffer>())
    {
        size_t formatter_count = 0;
        if (g_conf_data != nullptr && g_conf_data->thread_count > 0) {
            formatter_count = g_conf_data->thread_count;
        } else {
            formatter_count = std::thread::hardware_concurrency();
            if (formatter_count == 0) formatter_count = 2;
        }
        
        formatter_threads_active_ = formatter_count;

        for (size_t i = 0; i < formatter_count; ++i) {
            formatter_threads_.emplace_back(&LogPipeline::FormatterThreadEntry, this);
        }
        io_thread_ = std::thread(&LogPipeline::IOThreadEntry, this);
        main_loop_thread_ = std::thread(&LogPipeline::MainLoopThreadEntry, this);
    }

    ~LogPipeline() {
        Stop();
    }

    void Push(std::unique_ptr<LogMessage>&& msg) {
        if (stop_flag_.load(std::memory_order_acquire)) return;
        {
            std::unique_lock<std::mutex> lock(mtx_producer_);
            producer_buffer_->Push(std::move(msg));
        }
        cond_main_loop_.notify_one();
    }

private:
    void MainLoopThreadEntry() {
        while (!stop_flag_.load(std::memory_order_relaxed)) {
            std::unique_ptr<Buffer> buffer_to_process;
            {
                std::unique_lock<std::mutex> lock(mtx_producer_);
                // 采纳您的优化：使用 wait 即时响应
                cond_main_loop_.wait(lock, [this] {
                    return stop_flag_.load(std::memory_order_relaxed) || !producer_buffer_->IsEmpty();
                });

                if (stop_flag_.load(std::memory_order_relaxed)) {
                     // 在退出前，捕获最后一批日志
                    if (!producer_buffer_->IsEmpty()) {
                        buffer_to_process = std::move(producer_buffer_);
                    }
                } else if (!producer_buffer_->IsEmpty()) {
                    buffer_to_process = std::move(producer_buffer_);
                    producer_buffer_ = std::make_unique<Buffer>(); // 为生产者创建新缓冲区
                }
            }

            if (buffer_to_process) {
                std::unique_lock<std::mutex> lock(mtx_task_queue_);
                task_queue_.emplace(std::move(buffer_to_process));
                cond_task_queue_.notify_one();
            }
        }
    }

    void FormatterThreadEntry() {
        while (true) {
            std::unique_ptr<LogBatchTask> current_task_ptr;
            {
                std::unique_lock<std::mutex> lock(mtx_task_queue_);
                cond_task_queue_.wait(lock, [this]{
                    return stop_flag_.load(std::memory_order_relaxed) || !task_queue_.empty();
                });

                if (stop_flag_.load(std::memory_order_relaxed) && task_queue_.empty()) {
                    break; // 正确的退出条件
                }
                
                // 坚持安全、简单的“一次取一个任务”模型
                current_task_ptr = std::make_unique<LogBatchTask>(std::move(task_queue_.front()));
                task_queue_.pop();
            }

            // 安全地在锁外处理任务
            std::atomic<size_t> task_consume_idx(0);
            size_t task_size = current_task_ptr->buffer->Size();

            while (true) {
                size_t idx = task_consume_idx.fetch_add(1, std::memory_order_relaxed);
                if (idx >= task_size) break;
                
                const auto& msg_ptr = current_task_ptr->buffer->at(idx);

                if (msg_ptr->level_ >= LogLevel::value::ERROR)
                {
                    if (backup_thread_pool_) {
                        try {
                            // 格式化日志并提交给内部的备份线程池
                            std::string data_for_backup = msg_ptr->format();
                            backup_thread_pool_->enqueue(start_backup, data_for_backup);
                        } catch (const std::runtime_error&) {}
                    }
                }




                std::string formatted_str = msg_ptr->format();

                {
                    std::unique_lock<std::mutex> lock(mtx_reorder_);
                    reorder_buffer_[msg_ptr->sequence_id_] = std::move(formatted_str);
                } 
                cond_io_.notify_one(); // 总是通知IO线程
            }
        }
        formatter_threads_active_--; // 线程确认退出前，才递减计数
        cond_io_.notify_one();
    }

    void IOThreadEntry() {
        std::string batch_buffer;
        batch_buffer.reserve(4 * 1024);

        while (true) {
            {
                std::unique_lock<std::mutex> lock(mtx_reorder_);
                // 采纳您的优化：增加超时机制
                cond_io_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return (formatter_threads_active_.load(std::memory_order_relaxed) == 0 && reorder_buffer_.empty()) ||
                           reorder_buffer_.count(next_seq_to_write_);
                });

                if (formatter_threads_active_.load(std::memory_order_relaxed) == 0 && reorder_buffer_.empty()) {
                    break; 
                }

                while (reorder_buffer_.count(next_seq_to_write_)) {
                    batch_buffer.append(reorder_buffer_.at(next_seq_to_write_));
                    reorder_buffer_.erase(next_seq_to_write_);
                    next_seq_to_write_++;
                }
            }
            
            if (!batch_buffer.empty()) {
                // 采纳您的优化：添加错误处理
                for (const auto& flusher : flushers_) {
                    try {
                        flusher->Flush(batch_buffer.c_str(), batch_buffer.size());
                    } catch (const std::exception& e) {
                        // 可以添加错误日志记录，但要避免在日志系统内部无限递归
                        // fprintf(stderr, "Log flush error: %s\n", e.what());
                    }
                }
                batch_buffer.clear();
            }
        }
    }

    void Stop() {
        if (stop_flag_.exchange(true)) {
            return;
        }
        
        // 1. 唤醒并等待主循环线程结束
        cond_main_loop_.notify_all();
        main_loop_thread_.join();
        if (backup_thread_pool_) {
            // ThreadPool的析构函数会等待任务完成
            backup_thread_pool_.reset();
        }



        // 2. 唤醒所有格式化线程，让它们处理完队列中的剩余任务
        cond_task_queue_.notify_all();
        for (auto& t : formatter_threads_) {
            t.join();
        }
        
        // 3. 所有上游都已停止，最后唤醒并等待IO线程完成收尾工作
        cond_io_.notify_all();
        io_thread_.join();
    }

private:
    std::atomic<bool> stop_flag_;
    std::atomic<size_t> formatter_threads_active_{0};
    std::atomic<uint64_t> next_seq_to_write_{0};

    std::unique_ptr<Buffer> producer_buffer_;
    
    std::thread main_loop_thread_;
    std::vector<std::thread> formatter_threads_;
    std::thread io_thread_;

    std::mutex mtx_producer_;
    std::mutex mtx_task_queue_;
    std::mutex mtx_reorder_;
    std::condition_variable cond_main_loop_;
    std::condition_variable cond_task_queue_;
    std::condition_variable cond_io_;
    
    std::queue<LogBatchTask> task_queue_;
    std::map<uint64_t, std::string> reorder_buffer_;

    std::vector<LogFlush::ptr> flushers_;

    std::unique_ptr<ThreadPool> backup_thread_pool_;

};

} // namespace mylog