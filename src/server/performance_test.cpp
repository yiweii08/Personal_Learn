#include "../../log_system/logs_code/MyLog.hpp"
#include "../../log_system/logs_code/Util.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <sstream>

// 全局变量，用于初始化日志系统
// ThreadPool* tp = nullptr; 
mylog::Util::JsonData* g_conf_data;

void log_system_module_init()
{
    g_conf_data = mylog::Util::JsonData::GetJsonData();
    
    std::shared_ptr<mylog::LoggerBuilder> builder(new mylog::LoggerBuilder());
    builder->BuildLoggerName("performance_logger");
    builder->BuildLoggerFlush<mylog::RollFileFlush>("./perftest_log/test.log", 1024 * 1024 * 500);
    
    mylog::LoggerManager::GetInstance().AddLogger(builder->Build());
}

void worker_thread(mylog::AsyncLogger::ptr logger, size_t num_logs_to_write, std::atomic<size_t>& total_logs)
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    uint64_t thread_id_uint = std::stoull(ss.str());

    for (size_t i = 0; i < num_logs_to_write; ++i) {

        logger->Info("Performance test log message #%zu from thread %llu", i, thread_id_uint);
    }
    
    total_logs.fetch_add(num_logs_to_write, std::memory_order_relaxed);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <logs_per_thread>" << std::endl;
        return 1;
    }

    const size_t num_threads = std::stoul(argv[1]);
    const size_t logs_per_thread = std::stoul(argv[2]);
    const size_t total_logs_expected = num_threads * logs_per_thread;

    log_system_module_init();
    auto logger = mylog::GetLogger("performance_logger");
    if (!logger) {
        std::cerr << "Failed to get logger!" << std::endl;
        return 1;
    }

    std::cout << "Starting performance test with " << num_threads << " threads, "
              << logs_per_thread << " logs per thread." << std::endl;

    std::vector<std::thread> threads;
    std::atomic<size_t> total_logs_written(0);

    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, logger, logs_per_thread, std::ref(total_logs_written));
    }

    for (auto& t : threads) {
        t.join();
    }
    
    auto api_end_time = std::chrono::high_resolution_clock::now();

    std::cout << "All log APIs returned. Waiting for logs to be flushed to disk..." << std::endl;
    // 释放logger，析构和shuapan
    logger.reset();

    auto flush_end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> api_elapsed = api_end_time - start_time;
    std::chrono::duration<double> total_elapsed = flush_end_time - start_time;
    double throughput = total_logs_expected / api_elapsed.count();

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Test Finished." << std::endl;
    std::cout << "Total logs produced: " << total_logs_written << std::endl;
    std::cout << "Time for API calls: " << api_elapsed.count() << " seconds" << std::endl;
    std::cout << "Total time (including flush): " << total_elapsed.count() << " seconds" << std::endl;
    std::cout << "Throughput (API rate): " << static_cast<long long>(throughput) << " logs/second" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}
