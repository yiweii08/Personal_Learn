// 远程备份debug等级以上的日志信息-接收端
#include <string>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ServerBackupLog.hpp"
using std::cout;
using std::endl;
const std::string filename = "./logfile.log";
void usage(std::string procgress)
{
    cout << "usage error:" << procgress << "port" << endl;
}
bool file_exist(const std::string &name)
{
    struct stat exist;
    return (stat(name.c_str(), &exist) == 0);
}

void backup_log(const std::string &message)//用作回调
{
    FILE *fp = fopen(filename.c_str(), "ab");
    if (fp == NULL)
    {
        perror("fopen error: ");
        assert(false);
    }
    int write_byte = fwrite(message.c_str(), 1, message.size(), fp);
    if (write_byte != message.size())
    {
        perror("fwrite error: ");
        assert(false);
    }
    fflush(fp);
    fclose(fp);
}
int main(int args, char *argv[])
{
    if (args != 2)
    {
        usage(argv[0]);
        perror("usage error");
        exit(-1);
    }

    uint16_t port = atoi(argv[1]);
    std::unique_ptr<TcpServer> tcp(new TcpServer(port, backup_log));

    tcp->init_service();
    tcp->start_service();

    return 0;
}