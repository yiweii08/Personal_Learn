#include "../../log_system/logs_code/backlog/CliBackupLog.hpp" // 包含对应的头文件

#include <iostream>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno> // for errno

#include "../../log_system/logs_code/Util.hpp" // 引入Util来使用全局配置

// 声明外部全局配置变量
extern mylog::Util::JsonData *g_conf_data;

// 函数的完整实现放在 .cpp 文件中
void start_backup(const std::string &message)
{
    // 1. create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cout << __FILE__ << __LINE__ << "socket error : " << strerror(errno) << std::endl;
        perror(NULL);
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(g_conf_data->backup_port);
    inet_aton(g_conf_data->backup_addr.c_str(), &(server.sin_addr));

    int cnt = 5;
    while (-1 == connect(sock, (struct sockaddr *)&server, sizeof(server)))
    {
        std::cout << "正在尝试重连,重连次数还有: " << cnt-- << std::endl;
        if (cnt <= 0)
        {
            std::cout << __FILE__ << __LINE__ << "connect error : " << strerror(errno) << std::endl;
            close(sock);
            perror(NULL);
            return;
        }
    }

    // 3. 连接成功
    char buffer[1024];
    if (-1 == write(sock, message.c_str(), message.size()))
    {
        std::cout << __FILE__ << __LINE__ << "send to server error : " << strerror(errno) << std::endl;
        perror(NULL);
    }
    close(sock);
}