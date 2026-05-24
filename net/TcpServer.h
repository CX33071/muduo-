#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include "../base/noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <map>
#include "Acceptor.h"
#include "TcpConnection.h"
#include "EventLoopThreadpool.h"

namespace mulib
{
    namespace net
    {

        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        class TcpServer : noncopyable
        {
            using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
            using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
            using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
            using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
//
        public:
            TcpServer(EventLoop *loop, std::string nameArg, const InetAddress &listenAddr);
            ~TcpServer();

            void start();

            void setThreadNum(int numThreads);
            const std::string &ipPort() const { return ipPort_; }
            void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; };
            void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; };
            // const InetAddress &getListenAddress() const { return acceptor_->listenAddress(); }

        private:
            void newConnection(int sockfd, const InetAddress &peerAddr);
            void removeConnection(const TcpConnectionPtr &conn);
            void removeConnectionInLoop(const TcpConnectionPtr &conn);

            EventLoop *loop_;
            const std::string ipPort_;
            const std::string name_;
            std::unique_ptr<Acceptor> acceptor_;
            std::shared_ptr<EventLoopThreadPool> threadpool_;
            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;

            bool started_;
            int nextConnId_;
            ConnectionMap connections_;
        };

    }
}

#endif

//开始监听
//供conName使用
// namespace net
//服务器流程：
// TcpServer启动
// Acceptor,监听端口，连接器接受连接
// 有新的客户端连接时创建Tcpconnection
// 分配EventLoop
// Channel监听事件
// 收到数据handleRead,回调
// 发送数据handleWrite,回调
// 1. TcpServer 启动 2. Acceptor 开始监听端口 3. 客户端发起连接 4. Acceptor
//         接受连接，拿到新 sockfd 5. TcpServer::newConnection 创建
//             TcpConnection 6. 从线程池分配一个 EventLoop 7. 为 sockfd 创建
//                 Channel 8. Channel 注册读 /
//     写 / 关闭 /
//     错误事件监听 9. 等待客户端数据到达
//     10. 收到数据 → 触发 handleRead 读入 inputBuffer → 回调业务 MessageCallback
//     11. 业务发送数据 → 调用 conn->send()
//         可直接发就直接 write 发不完放进 outputBuffer，开启可写监听
//     12. 内核可写 → 触发 handleWrite 不断把 outputBuffer 数据发完
//     发完关闭写监听，触发 WriteCompleteCallback
//     13. 客户端断开 → 触发 handleClose TcpServer 移除连接，安全销毁 TcpConnection
