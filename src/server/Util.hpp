#pragma once
#include "jsoncpp/json/json.h"
#include <cassert>
#include <sstream>
#include <memory>
#include "bundle.h"
#include "Config.hpp"
#include <iostream>
#include <experimental/filesystem>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <fstream>
#include "../../log_system/logs_code/MyLog.hpp"

namespace storage
{
    namespace fs = std::experimental::filesystem;

    static unsigned char ToHex(unsigned char x)
    {
        return x > 9 ? x + 55 : x + 48;
    }

    static unsigned char FromHex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z')
            y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z')
            y = x - 'a' + 10;
        else if (x >= '0' && x <= '9')
            y = x - '0';
        else
            assert(0);
        return y;
    }
    static std::string UrlDecode(const std::string &str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            // if (str[i] == '+')
            //     strTemp += ' ';
            if (str[i] == '%')
            {
                assert(i + 2 < length);
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high * 16 + low;
            }
            else
                strTemp += str[i];
        }
        return strTemp;
    }

    class FileUtil
    {
    private:
        std::string filename_;

    public:
        FileUtil(const std::string &filename) : filename_(filename) {}

        //  获取文件大小
        int64_t FileSize()
        {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                mylog::GetLogger("asynclogger")->Info("%s, Get file size failed: %s", filename_.c_str(),strerror(errno));
                return -1;
            }
            return s.st_size;
        }
        // 获取文件最近访问时间
        time_t LastAccessTime()
        {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                mylog::GetLogger("asynclogger")->Info("%s, Get file access time failed: %s", filename_.c_str(),strerror(errno));
                return -1;
            }
            return s.st_atime;
        }

        // 获取文件最近修改时间
        time_t LastModifyTime()
        {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                mylog::GetLogger("asynclogger")->Info("%s, Get file modify time failed: %s",filename_.c_str(), strerror(errno));
                return -1;
            }
            return s.st_mtime;
        }

        // 从路径中解析出文件名
        std::string FileName()
        {
            auto pos = filename_.find_last_of("/");
            if (pos == std::string::npos)
                return filename_;
            return filename_.substr(pos + 1, std::string::npos);
        }

        // 从文件POS处获取len长度字符给content
        bool GetPosLen(std::string *content, size_t pos, size_t len)
        {
            // 判断要求数据内容是否符合文件大小
            if (pos + len > FileSize())
            {
                mylog::GetLogger("asynclogger")->Info("needed data larger than file size");
                return false;
            }

            // 打开文件
            std::ifstream ifs;
            ifs.open(filename_.c_str(), std::ios::binary);
            if (ifs.is_open() == false)
            {
                mylog::GetLogger("asynclogger")->Info("%s,file open error",filename_.c_str());
                return false;
            }

            // 读入content
            ifs.seekg(pos, std::ios::beg); // 更改文件指针的偏移量
            content->resize(len);
            ifs.read(&(*content)[0], len);
            if (!ifs.good())
            {
                mylog::GetLogger("asynclogger")->Info("%s,read file content error",filename_.c_str());
                ifs.close();
                return false;
            }
            ifs.close();

            return true;
        }

        // 获取文件内容
        bool GetContent(std::string *content)
        {
            return GetPosLen(content, 0, FileSize());
        }

        // 写文件
        bool SetContent(const char *content, size_t len)
        {
            std::ofstream ofs;
            ofs.open(filename_.c_str(), std::ios::binary);
            if (!ofs.is_open())
            {
                mylog::GetLogger("asynclogger")->Info("%s open error: %s", filename_.c_str(), strerror(errno));
                return false;
            }
            ofs.write(content, len);
            if (!ofs.good())
            {
                mylog::GetLogger("asynclogger")->Info("%s, file set content error",filename_.c_str());
                ofs.close();
            }
            ofs.close();
            return true;
        }

        // 压缩操作
        //  压缩文件
        bool Compress(const std::string &content, int format)
        {

            std::string packed = bundle::pack(format, content);
            if (packed.size() == 0)
            {
                mylog::GetLogger("asynclogger")->Info("Compress packed size error:%d", packed.size());
                return false;
            }
            // 将压缩的数据写入压缩包文件中
            FileUtil f(filename_);
            if (f.SetContent(packed.c_str(), packed.size()) == false)
            {
                mylog::GetLogger("asynclogger")->Info("filename:%s, Compress SetContent error",filename_.c_str());
                return false;
            }
            return true;
        }
        bool UnCompress(std::string &download_path)
        {
            // 将当前压缩包数据读取出来
            std::string body;
            if (this->GetContent(&body) == false)
            {
                mylog::GetLogger("asynclogger")->Info("filename:%s, uncompress get file content failed!",filename_.c_str());
                return false;
            }
            // 对压缩的数据进行解压缩
            std::string unpacked = bundle::unpack(body);
            // 将解压缩的数据写入到新文件
            FileUtil fu(download_path);
            if (fu.SetContent(unpacked.c_str(), unpacked.size()) == false)
            {
                mylog::GetLogger("asynclogger")->Info("filename:%s, uncompress write packed data failed!",filename_.c_str());
                return false;
            }
            return true;
        }
        // 目录操作
        // 以下三个函数使用c++17中文件系统给的库函数实现
        bool Exists()
        {
            return fs::exists(filename_);
        }

        bool CreateDirectory()
        {
            if (Exists())
                return true;
            return fs::create_directories(filename_);
        }

        bool ScanDirectory(std::vector<std::string> *arry)
        {
            for (auto &p : fs::directory_iterator(filename_))
            {
                if (fs::is_directory(p) == true)
                    continue;
                // relative_path带有路径的文件名
                arry->push_back(fs::path(p).relative_path().string());
            }
            return true;
        }
    };

    class JsonUtil
    {
    public:
        static bool Serialize(const Json::Value &val, std::string *str)
        {
            // 建造者生成->建造者实例化json写对象->调用写对象中的接口进行序列化写入str
            Json::StreamWriterBuilder swb;
            swb["emitUTF8"] = true;
            std::unique_ptr<Json::StreamWriter> usw(swb.newStreamWriter());
            std::stringstream ss;
            if (usw->write(val, &ss) != 0)
            {
                mylog::GetLogger("asynclogger")->Info("serialize error");
                return false;
            }
            *str = ss.str();
            return true;
        }
        static bool UnSerialize(const std::string &str, Json::Value *val)
        {
            // 操作方法类似序列化
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> ucr(crb.newCharReader());
            std::string err;
            if (ucr->parse(str.c_str(), str.c_str() + str.size(), val, &err) == false)
            {
                mylog::GetLogger("asynclogger")->Info("parse error");
                return false;
            }
            return false;
        }
    };
}
