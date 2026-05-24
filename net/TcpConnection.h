#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "../base/noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Buffer.h"

namespace mulib{//
    namespace net{
        class Buffer;
        class TcpConnection : noncopyable,
        public std::enable_shared_from_this<TcpConnection>{
        public:
            using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
            using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
            using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
            using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
            using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
            using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

            TcpConnection(EventLoop *loop, std::string conName, int sockfd, InetAddress localAddr, InetAddress peerAddr);
            ~TcpConnection();

            EventLoop *getLoop() const { return loop_; };
            const std::string &name() const { return name_; }
            const InetAddress &localAddress() const { return localAddr_; }
            const InetAddress &peerAddress() const { return peerAddr_; }
            bool connected() const { return state_ == kConnected; }
            bool disconnected() const { return state_ == kDisconnected; }
            void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = cb; }
            void setMessageCallback(MessageCallback cb) { messageCallback_ = cb; }
            void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = cb; }
            void setCloseCallback(CloseCallback cb) { closeCallback_ = cb; }
            void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark){
                highWaterMarkCallback_ = cb;
                highWaterMark_ = highWaterMark;
            } // 当发送缓冲区大小超过 highWaterMark 阈值时触发

            void connectEstablished();
            void connectDestroyed();
            void send(const std::string &message);

            void shutdown();
            void forceClose();

        private:
            enum StateE
            {
                kConnecting,
                kConnected,
                kDisconnecting,
                kDisconnected
            };
            void setState(StateE s) { state_ = s; };
            void handleRead(Timestamp receiveTime);
            void handleClose();
            void handleWrite();
            void handleError();
            void sendInLoop(const std::string &msg);
            void shutdownInLoop();
            const char *stateToString() const;
            EventLoop *loop_; // 此连接所属的 EventLoop
            std::string name_;
            StateE state_;
            std::unique_ptr<Socket> socket_;
            std::unique_ptr<Channel> channel_; // 事件分发器，监控 fd 上的事件（读写）
            InetAddress localAddr_;
            InetAddress peerAddr_;

            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            CloseCallback closeCallback_;
            HighWaterMarkCallback highWaterMarkCallback_;

            size_t highWaterMark_;
            Buffer inputBuffer_;
            Buffer outputBuffer_;
        };
    }
}

#endif

//不能拷贝赋值、安全获取自身shared_ptr回调时保证对象不被销毁
//连接建立/断开时调用
//收到消息时调用
//连接关闭时调用通知上层
//高水位时调用
//本次连接的名字,服务器名-连接端口-序列号，供日志用
//缓冲区太大时提醒用户
//半关闭连接，写完数据再关
//强制关闭连接
//服务端的localAddr就是服务端绑定地址，对端就是客户端地址，分服务端和客户端的Tcpconnection
//高水位限制值
// namespace net
// 读事件handleRead主动调用
// 写事件用send,send可能内核缓冲区满了一次性发不完，handlewrite用来自动发outputBuffer临时存的数据
