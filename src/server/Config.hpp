#pragma once
#include "Util.hpp"
#include <memory>
#include <mutex>
// 该类用于读取配置文件信息
namespace storage
{
    // 懒汉模式
    const char *Config_File = "Storage.conf";
    class Config
    {
    private:
        int server_port_;
        std::string server_ip;
        std::string download_prefix_; // URL路径前缀
        std::string deep_storage_dir_;     // 深度存储文件的存储路径
        std::string low_storage_dir_;     // 浅度存储文件的存储路径
        std::string storage_info_;     // 已存储文件的信息
        int bundle_format_;//深度存储的文件后缀，由选择的压缩格式确定
    private:
        static std::mutex _mutex;
        static Config *_instance;
        Config()
        {
            if (ReadConfig() == false)
            {
                mylog::GetLogger("asynclogger")->Fatal("ReadConfig failed");
                return;
            }
            mylog::GetLogger("asynclogger")->Info("ReadConfig complicate");
        }

    public:
        // 读取配置文件信息
        bool ReadConfig()
        {
            mylog::GetLogger("asynclogger")->Info("ReadConfig start");

            storage::FileUtil fu(Config_File);
            std::string content;
            if (!fu.GetContent(&content))
            {
                return false;
            }

            Json::Value root;
            storage::JsonUtil::UnSerialize(content, &root); // 反序列化，把内容转成jaon value格式

            // 要记得转换的时候用上asint，asstring这种函数，json的数据类型是Value。
            server_port_ = root["server_port"].asInt();
            server_ip = root["server_ip"].asString();
            download_prefix_ = root["download_prefix"].asString();
            storage_info_ = root["storage_info"].asString();
            deep_storage_dir_ = root["deep_storage_dir"].asString();
            low_storage_dir_ = root["low_storage_dir"].asString();
            bundle_format_ = root["bundle_format"].asInt();
            
            return true;
        }
        int GetServerPort()
        {
            return server_port_;
        }
        std::string GetServerIp()
        {
            return server_ip;
        }
        std::string GetDownloadPrefix()
        {
            return download_prefix_;
        }
        int GetBundleFormat()
        {
            return bundle_format_;
        }
        std::string GetDeepStorageDir()
        {
            return deep_storage_dir_;
        }
        std::string GetLowStorageDir()
        {
            return low_storage_dir_;
        }
        std::string GetStorageInfoFile()
        {
            return storage_info_;
        }

    public:
        // 获取单例类对象
        static Config *GetInstance()
        {
            if (_instance == nullptr)
            {
                _mutex.lock();
                if (_instance == nullptr)
                {
                    _instance = new Config();
                }
                _mutex.unlock();
            }
            return _instance;
        }
    };
    // 静态成员初始化，先写类型再写类域
    std::mutex Config::_mutex;
    Config *Config::_instance = nullptr;
}