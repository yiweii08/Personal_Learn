
#define DEBUG_LOG
#include "Service.hpp"
#include <thread>
using namespace std;

storage::DataManager *data_;
ThreadPool* tp=nullptr;
mylog::Util::JsonData* g_conf_data;
void service_module()
{
    storage::Service s;
    mylog::GetLogger("asynclogger")->Info("service step in RunModule");
    s.RunModule();
}

void log_system_module_init()
{
    g_conf_data = mylog::Util::JsonData::GetJsonData();
    tp = new ThreadPool(g_conf_data->thread_count);
    std::shared_ptr<mylog::LoggerBuilder> Glb(new mylog::LoggerBuilder());
    Glb->BuildLoggerName("asynclogger");
    Glb->BuildLoggerFlush<mylog::RollFileFlush>("./logfile/RollFile_log",
                                              1024 * 1024);

    mylog::LoggerManager::GetInstance().AddLogger(Glb->Build());
}
int main()
{
    log_system_module_init();
    data_ = new storage::DataManager();

    thread t1(service_module);

    t1.join();
    delete(tp);
    return 0;
}
