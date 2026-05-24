#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "../base/noncopyable.h"
#include "InetAddress.h"
#include <functional>
#include <memory>

namespace mulib{
    namespace net{//
        class Channel;
        class EventLoop;
        class Connector : noncopyable,public std::enable_shared_from_this<Connector>{
        public:
            using NewConnectionCallback = std::function<void(int sockfd)>;
            Connector(EventLoop *loop, const InetAddress &serverAddr);
            ~Connector();
            void setNewConnectionCallback(const NewConnectionCallback &cb){newConnectionCallback_ = cb;}
            void start();
            void restart();
            void stop();
            const InetAddress &serverAddress() const { return serverAddr_; }
        private:
            enum States
            {
                kDisconnected,
                kConnecting,
                kConnected
            };
            static const int kMaxRetryDelayMs = 30 * 1000;
            static const int kInitRetryDelayMs = 500;
            void setState(States s) { state_ = s; }
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();
            EventLoop *loop_;  // 客户端所在的事件循环
            InetAddress serverAddr_;  // 服务器地址
            bool connect_; // 是否正在连接
            States state_;   // 连接状态
            std::unique_ptr<Channel> channel_;  // 指向客户端的channel
            NewConnectionCallback newConnectionCallback_; // 连接成功后的回调
            int retryDelayMs_;    // 重试的延迟时间
        };
    }
}
#endif // MUDUO_NET_CONNECTOR_H

//TCP客户端核心
//安全使用shared_ptr管理自己
//客户端事件循环、要连接的服务器地址
//开始连接
//重连
//未连接
//连接中 
//已连接
//用状态及保证不会重复连接，不会乱序
//最大重试间隔30s
//初始间隔500ms
//指数退避重试，连接失败500ms->1s->...->30s
//设置状态
//真正开始连接，在IO线程执行,外部调用start(),转到startInLoop()
//真正停止连接,外部调用stop
//发起连接
//连接中处理
//连接成功，可写事件触发
//连接出错
//连接失败，重试
//移除并重置channel
//重置channel
//客户端所在事件循环
//服务器地址
//是否正在连接
//连接状态
//指向客户端的channel
//连接成功后的回调
//重试的延迟时间
// namespace net
