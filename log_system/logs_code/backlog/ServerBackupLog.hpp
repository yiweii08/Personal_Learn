#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>

using std::cout;
using std::endl;

using func_t = std::function<void(const std::string &)>;
const int backlog = 32;

class TcpServer;
//该类包含客户端的ip和端口信息
class ThreadData
{
public:
    ThreadData(int fd, const std::string &ip, const uint16_t &port, TcpServer *ts)
        : sock(fd),client_ip(ip),client_port(port),ts_(ts){}

public:
    int sock;
    std::string client_ip;
    uint16_t client_port;
    TcpServer *ts_;
};

class TcpServer
{
public:
    TcpServer(uint16_t port, func_t func)
        : port_(port), func_(func)
    {
    }
    void init_service()
    {
        // 创建
        listen_sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock_ == -1){
            std::cout << __FILE__ << __LINE__ <<"create socket error"<< strerror(errno)<< std::endl;
        }

        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_port = htons(port_);
        local.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listen_sock_, (struct sockaddr *)&local, sizeof(local)) < 0){
            std::cout << __FILE__ << __LINE__ << "bind socket error"<< strerror(errno)<< std::endl;
        }

        if (listen(listen_sock_, backlog) < 0) {
            std::cout << __FILE__ << __LINE__ <<  "listen error"<< strerror(errno)<< std::endl;
        }
    }

    static void *threadRoutine(void *args)
    {
        pthread_detach(pthread_self()); // 防止在start_service处阻塞

        ThreadData *td = static_cast<ThreadData *>(args);
        std::string client_info = td->client_ip + ":" + std::to_string(td->client_port);
        td->ts_->service(td->sock,move(client_info));
        close(td->sock);
        delete td;
        return nullptr;
    }

    void start_service()
    {
        while (true)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addrlen = sizeof(client_addr);
            int connfd = accept(listen_sock_, (struct sockaddr *)&client_addr, &client_addrlen);
            if (connfd < 0){
                std::cout << __FILE__ << __LINE__ << "accept error"<< strerror(errno)<< std::endl;
                continue;
            }

            // 获取client端信息
            std::string client_ip = inet_ntoa(client_addr.sin_addr); // 网络序列转字符串
            uint16_t client_port = ntohs(client_addr.sin_port);

            // 多个线程提供服务
            // 传入线程数据类型来访问threadRoutine，因为该函数是static的，所以内部传入了data类型存了tcpserver类型
            pthread_t tid;
            ThreadData *td = new ThreadData(connfd, client_ip, client_port, this);
            pthread_create(&tid, nullptr, threadRoutine, td);
        }
    }

    void service(int sock,const std::string&& client_info)
    {
        char buf[1024];

        int r_ret = read(sock, buf, sizeof(buf));
        if(r_ret ==-1){
            std::cout << __FILE__ << __LINE__ <<"read error"<< strerror(errno)<< std::endl;
            perror("NULL");
        }else if(r_ret > 0){
            buf[r_ret] = 0;
            std::string tmp = buf;
            func_(client_info+tmp); // 进行回调
        }
    }
    ~TcpServer()=default;

private:
    int listen_sock_;
    uint16_t port_;
    func_t func_;
};