#include "../logs_code/MyLog.hpp"
#include "../logs_code/ThreadPoll.hpp"
#include "../logs_code/Util.hpp"
using std::cout;
using std::endl;

ThreadPool* tp=nullptr;
mylog::Util::JsonData* g_conf_data;
void test() {
    int cur_size = 0;
    int cnt = 1;
    while (cur_size++ < 2) {
        mylog::GetLogger("asynclogger")->Info("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Warn("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Debug("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Error("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Fatal("测试日志-%d", cnt++);
    }
}

void init_thread_pool() {
    tp = new ThreadPool(g_conf_data->thread_count);
}
int main() {
    g_conf_data = mylog::Util::JsonData::GetJsonData();
    init_thread_pool();
    std::shared_ptr<mylog::LoggerBuilder> Glb(new mylog::LoggerBuilder());
    Glb->BuildLoggerName("asynclogger");
    Glb->BuildLoggerFlush<mylog::FileFlush>("./logfile/FileFlush.log");
    Glb->BuildLoggerFlush<mylog::RollFileFlush>("./logfile/RollFile_log",
                                              1024 * 1024);
    //建造完成后，日志器已经建造，由LoggerManger类成员管理诸多日志器
    // 把日志器给管理对象，调用者通过调用单例管理对象对日志进行落地
    mylog::LoggerManager::GetInstance().AddLogger(Glb->Build());
    test();
    delete(tp);
    return 0;
}