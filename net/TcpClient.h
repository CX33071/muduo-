#pragma once

#include <mutex>
#include "../base/Timestamp.h"
#include "Connector.h"
#include "TcpConnection.h"
namespace muduo{
    namespace net{
        class TcpClient{
            public:
             using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
             using ConnectorPtr = std::shared_ptr<Connector>;
             using COnnectionCallback =
                 std::function<void(const TcpCOnnection&)>;
             using MessageCallback = std::function<
                 void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
             TcpClient(EventLoop* loop, const INetAddress& serverAddr);
             ~TcpClient();
             void setConnectionCallback(const ConnectionCallback& cb){
                 connectionCallback_ = cb;
             }
        }
        void setMessageCallback(const MessageCallback&cb){
            messageCallback_ = cb;
        }
        void setWriteCompleteCallback(const WriteCompleteCallback&cb){
            writeCompleteCallback_ = cb;
        }
        void connect();
        void disconnect();
        void stop();
        TcpClient::TcpConnectionPtr connection();//获取当前连接
        private:
         void newConnection(int sockfd);//连接成功，创建TcpConnection
         void removeConnection(const TcpCOnnectionPtr& conn);
         bool connect_;
         bool retry_;//断开是否重连
         int nextConnId_;
         EventLoop* lopp_;
         ConnectorPtr connector_;
         ConnectionCallback connectionCallback_;
         MessageCallback messageCallback_;
         WriteCompleteCallback writeCompleteCallback_;
         TcpConnectionPtr connection_;//保存与服务器的连接
         std::mutex mutex_;
         };  // namespace net
}