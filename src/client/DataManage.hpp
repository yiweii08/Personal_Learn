#pragma once
#include<unordered_map>
#include"Util.hpp"

// 这个就是没变

namespace my_storage {
	class DataManager {
	private:
		std::string storage_file_;

		std::unordered_map<std::string, std::string> table_;
	public:
		DataManager(const std::string storage_file) :storage_file_(storage_file) {
			InitLoad();
		}
	public:
		void Split(const std::string& body, const std::string& sep, std::vector<std::string>* arr) {
			//Format: key + " " + val + \n, representing the storage info of a file
			size_t pos = 0;
			size_t begin = 0;
			while (begin < body.size()) {
				pos = body.find(sep, begin);
				if (pos < body.size()) {
					//Indicates that there is a file storage info
					arr->emplace_back(body.begin() + begin, body.begin() + pos);
					begin = pos + 1;
				}
				else {
					arr->emplace_back(body.begin() + begin, body.end());
					begin = body.size();
				}
			}
		}
		//Load stored information data generated from previously uploaded files
		bool InitLoad() {
			FileUtil fu(storage_file_);
			//if no storage file exists, don't need to load
			if (fu.Exists() == false) return true;
			std::string content;
			fu.GetContent(&content);

			//Parse file content to get storage info for each file
			std::vector<std::string> arry;
			Split(content, "\n", &arry);
			for (auto& e : arry) {
				std::vector<std::string> s;
				Split(e, " ", &s);
				table_[s[0]] = s[1];
			}
			return true;
		}
		bool Storage() {
			// Load the data in table_ into storage_file_
			std::stringstream ss;
			for (auto e : table_) {
				ss << e.first << " " << e.second << "\n";// Format: file name + space + unique identifier
			}
			FileUtil fu(storage_file_);
			fu.SetContent(ss.str());
			return true;
		}
		bool Insert(const std::string& key, const std::string& val) {
			table_[key] = val;
			Storage();
			return true;
		}
		bool Update(const std::string& key, const std::string& val) {
			table_[key] = val;
			Storage();
			return true;
		}
		bool GetOneByKey(const std::string& key, std::string* val) {
			auto it = table_.find(key);
			if (it != table_.end()) {
				*val = it->second;
				return true;
			}
			return false;
		}
	};

}
