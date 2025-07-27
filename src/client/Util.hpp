#pragma once
#include <sstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <fstream>

namespace my_storage
{
	namespace fs = std::filesystem;
	class FileUtil
	{
	private:
		std::string filename_;

	public:
		FileUtil(const std::string& filename) : filename_(filename) {}

		size_t FileSize()
		{
			struct stat s;
			auto ret = stat(filename_.c_str(), &s);
			if (ret == -1)
			{
				std::cout << __FILE__ << __LINE__ << "Get file size failed" << strerror(errno) << std::endl;
				return -1;
			}
			return s.st_size;
		}
		time_t LastAccessTime()
		{
			struct stat s;
			auto ret = stat(filename_.c_str(), &s);
			if (ret == -1)
			{
				std::cout << __FILE__ << __LINE__ << "Get file access time failed" << strerror(errno) << std::endl;
				return -1;
			}
			return s.st_atime;
		}

		time_t LastModifyTime()
		{
			struct stat s;
			auto ret = stat(filename_.c_str(), &s);
			if (ret == -1)
			{
				std::cout << __FILE__ << __LINE__ << "Get file modify time failed" << strerror(errno) << std::endl;
				return -1;
			}
			return s.st_mtime;
		}

		std::string FileName()
		{
			auto pos = filename_.find_last_of("/");
			if (pos == std::string::npos)
				return filename_;
			return filename_.substr(pos + 1, std::string::npos);
		}

		// read from pos
		bool GetPosLen(std::string* content, size_t pos, size_t len)
		{
			if (pos + len > FileSize())
			{
				std::cout <<__FILE__<<__LINE__<< "needed data is larger file fize" << std::endl;
				return false;
			}

			std::ifstream ifs;
			ifs.open(filename_.c_str(), std::ios::binary);
			if (ifs.is_open() == false)
			{
				std::cout << __FILE__ << __LINE__<< "file open error" << std::endl;
				return false;
			}

			ifs.seekg(pos, std::ios::beg);
			content->resize(len);
			ifs.read(&(*content)[0], len);
			if (!ifs.good())
			{
				std::cout << __FILE__ << __LINE__ << "-"
					<< "read file content error" << std::endl;
				ifs.close();
				return false;
			}
			ifs.close();

			return true;
		}

		bool GetContent(std::string* content)
		{
			return GetPosLen(content, 0, FileSize());
		}

		//write file
		bool SetContent(const std::string& content)
		{
			std::ofstream ofs;
			ofs.open(filename_.c_str(), std::ios::binary);
			if (!ofs.is_open())
			{
				std::cout << __FILE__ << __LINE__ << "file open error" << std::endl;
				return false;
			}
			ofs.write(content.c_str(), content.size());
			if (!ofs.good())
			{
				std::cout << __FILE__ << __LINE__ << "file set content error" << std::endl;
				ofs.close();
			}
			ofs.close();
			return true;
		}

		//there is three function about dir
		//c++17，need include <filesystem>. namespace fs = std::filesystem;
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

		bool ScanDirectory(std::vector<std::string>* arry)
		{
			FileUtil(filename_).CreateDirectory();
			for (auto& p : fs::directory_iterator(filename_))
			{
				if (fs::is_directory(p) == true)
					continue;
				arry->push_back(fs::path(p).relative_path().string());
			}
			return true;
		}
	};
}