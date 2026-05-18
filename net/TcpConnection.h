#pragma once
#include "../base/noncopyable.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
namespace mduo{
    namespace net{
    class Buffer;
    class TcpConnection : noncopyable,public std::enable_shared_from_this<TcpConnection>{//不能拷贝赋值、安全获取自身shared_ptr回调时保证对象不被销毁
        public:
         using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
         using ConnectionCallback = std::function<void(const TcpConnection&)>;//连接建立/断开时调用
         using MessageCallback =
             std::function<void(const TcpConnectionPtr&, Buufer*, Timestamp)>;//收到消息时调用
         using WriteCompleteCallback =
             std::function<void(const TcpConnectionPtr&)>;
         using CloseCallback = std::function<void(const TcpConnectionPtr&)>;//连接关闭时调用通知上层
         using HighWateMarkCallback =
             std::function<void(const TcpConnectionPtr&, size_t)>;//高水位时调用
         TcpConnection(EventLoop* loop,
                       std::string conName,//本次连接的名字,服务器名-连接端口-序列号，供日志用
                       int sockfd,
                       InetAddress localAddr,
                       InetAddress peerAddr);
         ~TcpConnection();
         EventLoop* getLoop() const { return loop_; }
         const std::string& name() const { return name_; }
         const InetAddress& localAddress() const { return localAddr_; }
         const InetAddress& peerAddress() const { return peerAddr_; }
         bool connected() const { return state_ == kConnected; }
         bool disconnected() const { return state_ == kDisconnected; }
         void setConnectionCallback(ConnectionCallback cb){
             connectionCallback_ = cb;
         }
         void setMessageCallback(MessageCallback cb) { messageCallbakc_ = cb; }
         void setWriteCompleteCallback(WriteCompleteCallback cb){
             writeCompleteCallback_ = cb;
         }
         void setHighWaterMarkCallback(const HighWaterMarkCallback&cb,size_t highWaterMark){
             gighWaterMarkCallback_ = cb;
             highWaterMark_ = highWaterMark;
         }//缓冲区太大时提醒用户
         void connectEstablelished();
         void connecctDestroyed();
         void shutdown();//半关闭连接，写完数据再关
         void forceClose();//强制关闭连接
        private:
         enum StateE { 
            kConnecting; 
            kConnected, 
            kDisconnecting, 
            kDisconnected };
         void setState(StateE s) { state_ = s; }
         void handleRead(Timestamp receiveTime);
         void handleClose();
         void handleWrite();
         void handleError();
         void sendInLoop(const std::string& msg);
         void shutdownInLoop();
         const char* stateToString() const;
         EventLoop* loop_;
         std::string name_;
         StateE state_;
         std::unique_ptr<Socket> socket_;
         std::unique_ptr<Channel> channel_;
         InetAddress localAddr_;//服务端的localAddr就是服务端绑定地址，对端就是客户端地址，分服务端和客户端的Tcpconnection
         InetAddress peerAddr_;
         ConnectionCallback connectionCallback_;
         MessageCallback messageCallback_;
         WriteCompleteCallback writeCompleteCallback_;
         CloseCallback closeCallback_;
         HighWaterMarkCallback highWaterMarkCallback_;
         size_t highWaterMark_;//高水位限制值
         Buffer inputBuffer_;
         Buffer outoutBuffer_;
    };
    }  // namespace net
}
// 读事件handleRead主动调用
// 写事件用send,send可能内核缓冲区满了一次性发不完，handlewrite用来自动发outputBuffer临时存的数据