#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "DataManage.hpp"
#include <string>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

using std::cout;
using std::endl;
using std::cerr;

const unsigned short server_port_g = 8081;
const std::string server_ip_g = "127.0.0.1";
const std::string storage_filename_g = "./storage.dat";//for storage uploaded files info

namespace my_storage {
	bool if_upload_success = false;

	class Storage {
	private:
		DataManager* dm_;
		std::string low_storage_dir_;
		std::string deep_storage_dir_;
	public:
		Storage(const std::string& low_storage_dir, const std::string& deep_storage_dir)
			:low_storage_dir_(low_storage_dir), deep_storage_dir_(deep_storage_dir) {
			dm_ = new DataManager(storage_filename_g);
		}

		std::string GetFileIdentifier(const std::string& filename) {
			FileUtil fu(filename);
			std::stringstream ss;
			ss << fu.FileName() << "-" << fu.FileSize() << "-" << fu.LastModifyTime();
			return ss.str();
		}
		static void upload_callback(struct evhttp_request* req, void* arg) {
			cout << "http_client_cb" << endl;
			event_base* bev = (event_base*)arg;
			if (req == NULL)
			{
				int errcode = EVUTIL_SOCKET_ERROR();
				cerr << "socket error " << evutil_socket_error_to_string(errcode) << endl;
				return;
			}
			int rsp_code = evhttp_request_get_response_code(req);
			if (rsp_code != HTTP_OK) {
				cerr << "response:" << rsp_code << endl;
				if_upload_success = false;
			}
			else {
				if_upload_success = true;
			}
			event_base_loopbreak(bev);
		}
		bool Upload(const std::string& filename) {
			FileUtil fu(filename);

			event_base* base = event_base_new();
			if (base == NULL) {
				cerr << __FILE__ << __LINE__ << "event_base_new() err" << endl;
				event_base_free(base);
				return false;
			}
			//bufferevent connect http server
			bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
			evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, NULL, bev, server_ip_g.c_str(), server_port_g);

			//http_client request callback set
			evhttp_request* req = evhttp_request_new(upload_callback, base);

			//set req header
			evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
			evhttp_add_header(output_headers, "Host", server_ip_g.c_str());
			evhttp_add_header(output_headers, "FileName", fu.FileName().c_str());
			evhttp_add_header(output_headers, "Content-Type", "application/octet-stream");
			if (filename.find("low_storage") != std::string::npos) {
				evhttp_add_header(output_headers, "StorageType", "low");
			}
			else {
				evhttp_add_header(output_headers, "StorageType", "deep");
			}

			//read filedata and make request
			std::string body;
			fu.GetContent(&body);
			evbuffer* output = evhttp_request_get_output_buffer(req);
			if (0 != evbuffer_add(output, body.c_str(), body.size())) {
				cerr << __FILE__ << __LINE__ << "evbuffer_add_printf() err" << endl;
			}

			if (0 != evhttp_make_request(evcon, req, EVHTTP_REQ_POST, "/upload")) {
				cerr << __FILE__ << __LINE__ << "evhttp_make_request() err" << endl;
			}

			if (base)
				event_base_dispatch(base);
			if (base)
				event_base_free(base);
			return true;
		}

		bool IfNeedUpload(const std::string& filename) {
			std::string old;
			if (dm_->GetOneByKey(filename, &old)==true) {//true indicates that the storage file info already exists
				if (GetFileIdentifier(filename) == old)
					return false;
			}

			// Upload a file that has not been modified for a while
			FileUtil fu(filename);
			if (time(NULL) - fu.LastModifyTime() < 5) {
				// just changed in 5 seconds，think the file is still being changed
				return false;
			}
			return true;
		}
		bool RunModule() {
			while (1) {
				//iterate to get all files in the specified folder,and judge need upload or not
				FileUtil low(low_storage_dir_);
				FileUtil deep(deep_storage_dir_);
				std::vector<std::string> arry;
				low.ScanDirectory(&arry);
				deep.ScanDirectory(&arry);
				//2. upload
				for (auto& a : arry) {
					if (IfNeedUpload(a) == false) {
						continue;
					}
					Upload(a);
					if (if_upload_success == true) {
						dm_->Insert(a, GetFileIdentifier(a));
						std::cout << a << " upload success!\n";
					}
					else {
						std::cout << a << " upload failed!\n";
					}
				}
				Sleep(1);//avoid waste cpu 
			}
		}
	};
}