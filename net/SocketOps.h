#pragma once
#include <arpa/inet.h>
#include <unistd.h>
namespace muduo{
    namespace net{
        namespace socket{
        int createNonblockingOrDie();//创建非阻塞socket
        void bindOrDie(int sockfd, const sockaddr_in& addr);
        void listenOrDie(int sockfd);
        int accept(int sockfd, sockaddr_in* addr);
        int acccept1(int sockfd, sockaddr_in* addr);
        void connect(int sockfd, const sockaddr_in& addr);
        void close(int sockfd);
        void setNonBlockAndCloseOnExec(int sockfd);
        void shutdownWrite(int sockfd);//关闭写端
        int getSocketError(int sockfd);//获取socket内部错误码
        sockaddr_in getLocalAddr(int sockfd);//获取本端地址
        sockaddr_in getPeerAddr(int sockfd);//获取对端地址
        bool isSelConnect(int sockfd);//判断是否自己连自己
        void fromHostPort(const char* ip,
                          uint16_t port,
                          struct sockaddr_in* addr);//ip+端口->sockaddr_in
        void toHostPort(char* buf, size_t size, const struct sockaddr_in& addr);//sockaddr_in->字符串，只含端口
        void toIpPort(char* buf, size_t size, const struct sockaddr* addr);//sockaddr->"ip:port"字符串
        void toIp(char* buf, size_t size, const struct sockaddr* addr);//只转IP字符串
        }  // namespace socket
    }
}