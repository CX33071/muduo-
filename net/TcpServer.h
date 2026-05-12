#pragma once
#include <map>
#include "../base/noncopyable.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadpool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
namespace muduo{
    namespace net{
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    class TcpServer:noncopyable{
        using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
        using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
        using MessageCallback =
            std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
        using WriteCompleteCallback =
            std::function<void(const TcpConnectionPtr&)>;
            public:
             TcpServer(EventLoop* loop,
                       std::string nameArg,
                       const InetAddress& listenAddr);
             ~TcpServer();
             void start();//开始监听
             void setThreadNum(int numThreads);
             const std::string& ipPort() const { return ipPort_; }
             void setConnectionCallback(const ConnectionCallback&cb){
                 connectionCallback_ = cb;
             }
             void setMessageCallback(const messageCallback&cb){
                 messageCallback_ = cb;
             }
             private:
              void newConnection(int sockfd, const InetAddress& peerAddr);
              void removeConnection(const TcpConnectionPtr& conn);
              void removeConnectionInLoop(const TcpconnectionPtr& conn);
              EventLoop* loop_;
              const std::string ipPort_;
              const std::string name_;
              std::unqiue_ptr<Acceptor> acceptor_;
              std::shared_ptr<EventLoopThreadPool> threadpool_;
              ConnectionCallback connectionCallback_;
              MessageCallback messageCallback_;
              WriteCompleteCallback writeCompleteCallback_;
              bool started_;
              int nextConnId_;//供conName使用
              ConnectionMap connections_;
    };
    }  // namespace net
}