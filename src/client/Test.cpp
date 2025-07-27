#include "Storage.hpp"

using std::cout;
using std::endl;

const std::string low_dir_g = "./low_storage/";
const std::string deep_dir_g = "./deep_storage/";

int main()
{
#ifdef _WIN32
	WSADATA wda;
	WSAStartup(MAKEWORD(2, 2), &wda);
#endif
	my_storage::Storage bu(low_dir_g, deep_dir_g);
	bu.RunModule();

	return 0;
}