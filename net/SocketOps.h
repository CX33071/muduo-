#ifndef MUDUO_NET_SOCKETOPS_H
#define MUDUO_NET_SOCKETOPS_H

#include <arpa/inet.h>
#include <unistd.h>//

namespace mulib{
    namespace net{
        namespace socket{
            int createNonblockingOrDie();
            void bindOrDie(int sockfd, const sockaddr_in &addr);
            void listenOrDie(int sockfd);
            int accept(int sockfd, sockaddr_in *addr);
            int accept1(int sockfd, sockaddr_in *addr);
            void close(int sockfd);
            int connect(int sockfd, const sockaddr_in &addr);

            void setNonBlockAndCloseOnExec(int sockfd);
            void shutdownWrite(int sockfd);

            int getSocketError(int sockfd);
            sockaddr_in getLocalAddr(int sockfd);
            sockaddr_in getPeerAddr(int sockfd);
            bool isSelfConnect(int sockfd);

            void fromHostPort(const char *ip, uint16_t port, struct sockaddr_in *addr);
            void toHostPort(char *buf, size_t size, const struct sockaddr_in &addr);
            void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
            void toIp(char *buf, size_t size, const struct sockaddr *addr);
        }
    }
}

#endif

//创建非阻塞socket
//关闭写端
//获取socket内部错误码
//获取本端地址
//获取对端地址
//判断是否自己连自己
//ip+端口->sockaddr_in
//sockaddr_in->字符串，只含端口
//sockaddr->"ip:port"字符串
//只转IP字符串
// namespace socket
