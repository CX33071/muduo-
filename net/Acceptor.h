#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <functional>
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"

namespace mulib{
    namespace net{
        class Acceptor{
        public:
            using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;//
            Acceptor(EventLoop *loop, const InetAddress &listenAddr);
            ~Acceptor();
            void setNewConnectionCallback(const NewConnectionCallback &cb);

            bool listening() const { return listening_; };
            void listen();

        private:
            void handleRead();

            EventLoop *loop_;
            Socket acceptSocket_;
            Channel acceptChannel_;
            NewConnectionCallback newConnectionCallback_;
            bool listening_;
        };
    }
}
#endif

// namespace net
//创建Acceptor,调用listen(),客户端发起连接有可读事件Channel调用handleRead(),handleread()调用accept获取新连接fd,调用回调函数
