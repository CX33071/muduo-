#include "TcpServer.h"
#include <assert.h>
#include "../base/logger.h"
using namespace mulib::net;
TcpServer::TcpServer(EventLoop*loop,std::string nameArg,const InetAddress&listenAddr):loop_(loop),name_(nameArg),ipPort_(listenAddr.toIpPoet()),acceptor_(new Acceptor(loop,listenAddr)),threadpool_(new EventLoopThreadPool(loop)),connectionCallback_().messageCallback_(),started_(false),nextConnId_(1){
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress& peerAddr) {
            newConnection(sockfd, peerAddr);
        });//Acceptor收到新连接，设置新连接回调
}
void TcpServer::start(){
    if(!started_){
        started_ = true;
        threadpool_->start();
    }
    if(!acceptor_->listening()){
        loop_->runInLoop([this] { acceptor_->listen(); });

    }
}
void TcpServer::setThreadNum(int numThreads){
    assert(numThreads>=0);
    threadpool_->setThreadNum(numThreads);
}
void TcpServer::newConnection(int sockfd,const INetAddress&peerAddress){
    loop_->assertInLoopThread();
    EventLoop* ioLoop = threadpool_->getNextLoop();//从线程池中取一个线程
    char buff[32];
    snprintf(buff, sizeof(buff), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId();
    std::string connName = name + buff;
    LOG_INFO << "TcpServer:newConnection[" << name_ << "] - new connection ["
             << connName << "] form " << peerAddr.toHostPort().c_str();
    InetAddress localAddr(socket::getLocalAddr(sockfd));
    TcpConnectionPtr conn(
        new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;//存入map
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeConnectionCallback_);
    conn->setCloseCallback(
        [this](const TcpConnectionPtr& conn) { removeConnection(conn); });
    ioLoop->runInLoop([conn] { conn->connectEstablished(); });
}
void TcpCOnnection::removeConnection(const TcpConnectionPtr&conn){
    loop_->runInLoop([this, conn] { removeConnectionInLoop(conn); });
}
void TcpConnection::removeConnectionInLoop(const TcpConnectionPtr&conn){
    loop_->assertInLoopThread();
    size_t n = connections_.erase(conn->name());//先从map中删除
    (void)n;//消除编译器未使用警告
    assert(n == 1);
    EventLoop* subLoop = conn->getLoop();
    subLoop->queueInLoop([conn] { conn->connectDestroyed(); });//销毁连接
}
TcpServer::~TcpServer(){
    for(auto&item:connections_){
        TcpConnectionPtr conn(item.second);//拷贝一份连接给局部变量conn,确保不会再操作的时候断开连接
        item.second.reset();//从map中删除
        conn->getLoop()-<runINLoop(std::bind(&TcpConnection::connectDestroyed,conn));//让连接自己的IO线程去安全销毁这个连接
    }
}