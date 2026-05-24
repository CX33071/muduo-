#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <string>
#include <arpa/inet.h>
#include <memory.h>
#include "SocketOps.h"

namespace mulib{
    namespace net{
        class InetAddress{
        public:
            explicit InetAddress(unsigned int port = 0);
            InetAddress(const std::string &ip, unsigned int port);
            InetAddress(const sockaddr_in &addr);

            const sockaddr_in &getSockAddr() const{
                return addr_;
            }
            socklen_t getSockLen() const { return sizeof(addr_); };
            std::string toHostPort() const;
            std::string toIpPort() const;

        private:
            sockaddr_in addr_;
        };
    }
}//
using namespace mulib::net;
inline InetAddress::InetAddress(unsigned int port){
    memset(&addr_, 0, getSockLen());
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = INADDR_ANY;
}
inline InetAddress::InetAddress(const std::string &ip, unsigned int port){
    memset(&addr_, 0, getSockLen());
    addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    addr_.sin_port = htons(port);
}
inline InetAddress::InetAddress(const sockaddr_in &addr){
    memcpy(&addr_, &addr, sizeof(addr_));
}
inline std::string InetAddress::toHostPort() const{
    char buf[32];
    socket::toHostPort(buf, sizeof(buf), addr_);
    return buf;
}
inline std::string InetAddress::toIpPort() const{
    char buf[64] = "";
    socket::toIpPort(buf, sizeof(buf), reinterpret_cast<const sockaddr *>(&getSockAddr()));
    return buf;
}
#endif

//地址类
//若没有传入参数则系统自动分配空闲窗口
