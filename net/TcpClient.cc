#include "TcpClient.h"
#include <assert.h>
#include <mutex>
#include <string>
#include "../base/logger.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketOps.h"
#include "TcpConnection.h"
using namespace muduo::net;
TcpClient::TcpClient(EventLoop*loop,const InetAddress&serverAddr):loop_(loop),connector(new connector(loop,serverAddr)),retry_(false),connect_(true),nextConnId_(1){
    connector_->setNewConnectionCallback(
        [this](int sockfd) { newConnection(sockfd); });
    LOG_DEBUG << "TcpClient::connect[" << this << "] - connecting to "
              << connector_->serverAddress().toHostPort();
    connect_=true;
    connector_->start();
}
void TcpClient::disconnect(){
    connect_=false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(connection_){
            connection_->shutdown();
        }
    }
}
void TcpClient::stop(){
    connect_=false;
    connector_->stop();
}
void TcpClient::newConnection(int sockfd){
    loop_->assertInLoopThread();
    InetAddress peerAddr(socket::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toHostPort().c_str(),
             nextConnId_);
    ++nextConnId_;
    std::string connName=buf;
    InetAddress localAddr(socket::getLocalAddr(sockfd));
    TcpConnectionPtr conn(
        new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));//，拿到sockfd,设置TcpConnection
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        [this](const TcpConnectionPtr& conn) { removeConnection(conn); });
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();//激活连接
}
void TcpClient::removeConnection(const TcpConnectionPtr&conn){
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());
    {
        std::unique_lock<std::mutex> lock(mutex_);
        assert(connection == conn);
        connection_.reset();  // 清空map
    }
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_) {
        LOG_INFO << "TcpClient::connect[" << this << "] - Reconnecting to "
                 << connector_->serverAddress().toHostPort();
        connector_->restart();
    }
}
TcpClient::~TcpClient(){
    LOG_INFO << "TcpClient::~TcpClient[" << this << "] - connector "
             << static_cast<const void*>(connector_.get());
    TcpCOnnectionPtr conn;
    bool unique = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        unique=connection_.unique();//true只有我掌握这个连接，false还有其他线程掌握这个连接
        conn = connection_;
    }
    if(conn){//如果连接存在
        assert(loop_=conn->getLoop());
        loop_->runInLoop([conn, loop = loop_, this]() {
            conn->setCloseCallback([loop, this](const TcpConnectionPtr& conn) {
                this->removeConnection(conn);
            });
        });
        if(unique){//只有我掌握这个连接
            conn->forceClose();//强制关闭连接
        }
    }else{
        connector_->stop();//停止连接器
        loop_->runAfter(1.0, [connector = connector_]() {});
    }
}
TcpCLient::TcpConnectionPtr TcpClient::connection(){
    std::unique_lock<std::mutex> lock(mutex_);
    return connection_;
}