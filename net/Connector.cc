#include "Connector.h"
#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketOps.h"
#include <assert.h>
using namespace mulib::net;

Connector::Connector(EventLoop*loop,const InetAddress&serverAddr) : loop_(loop),
serverAddr_(serverAddr),
connect_(false),
state_(kDisconnected),
retryDelayMs_(kInitRetryDelayMs){
    LOG_DEBUG << "ctor[" << this << "]";
}
Connector::~Connector(){
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!channel_);
}
void Connector::start(){
    connect_ = true;
    loop_->runInLoop([this]
                     { startInLoop(); });
}
void Connector::startInLoop(){
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_){
        connect();
    }
    else{
        LOG_DEBUG << "do not connect";
    }
}
void Connector::stop(){
    connect_ = false;
    loop_->queueInLoop([this]
                       { stopInLoop(); });
}
void Connector::stopInLoop(){
    loop_->assertInLoopThread();
    if (state_ == kConnecting){
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}
void Connector::connect(){
    int sockfd = socket::createNonblockingOrDie();
    int ret = socket::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
        socket::close(sockfd);
        break;

    default:
        LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
        socket::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}
void Connector::restart(){
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}
void Connector::connecting(int sockfd){
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this] { Connector::handleWrite(); });
    channel_->setErrorCallback([this] { Connector::handleError(); });
    channel_->enableWriting();
}
int Connector::removeAndResetChannel(){
    channel_->disableAll();
    int sockfd = channel_->fd();
    loop_->queueInLoop([this] { Connector::resetChannel(); });
    return sockfd;
}
void Connector::resetChannel(){
    channel_.reset();
}
void Connector::handleError(){
    LOG_ERROR << "Connector::handleError state=" << state_;
    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}
void Connector::handleWrite(){
    LOG_TRACE << "Connector::handleWrite " << state_;
    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        if (err){
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror(err);
            retry(sockfd);
        }
        else if(socket::isSelfConnect(sockfd)){
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else{
            setState(kConnected);
            if(connect_){
                newConnectionCallback_(sockfd);
            }
            else{
                socket::close(sockfd);
            }
        }
    }
    else{
        assert(state_ == kDisconnected);
    }
}
const int Connector::kMaxRetryDelayMs;
void Connector::retry(int sockfd){
    socket::close(sockfd);
    setState(kDisconnected);
    if(connect_){
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toHostPort() << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_ / 1000.0, [this]
                        { startInLoop(); });
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else{
        LOG_DEBUG << "do not connect";
    }
}//

//绑定事件循环、要连接的服务器地址、初始不连接、初始状态：未连接、初始重试等待500ms
//确保channel已经清理
//扔到IO线程执行,开始尝试连接服务器
//确保在IO线程
///适配单线程客户端和多线程客户端，如果是单线程，业务代码和eventloop在同一个线程，直接原第执行，不会跨线程；否则业务在主线程eventloop在子线程，这是runinloop会把任务塞进队列
//确保状态是未连接
//真正发起连接
//外部停止接口
//如果正在连接
//该状态未连接
//移除channel
//关闭sockfd,不重试
//非阻塞连接，正常
//进入连接中
//连接被拒绝
//网络不可达
//重试
//致命错误，关闭
// connectErrorCallback_();
//进入连接中状态
//状态：连接中
// 创建channel
//可写事件=连接完成
//错误事件
//监听可写
//此时连接已经完成，不论成功与否，只是状态还没更新
//自动重连：从来没有连接成功过
//关闭旧的sockfd
//延迟一段时间再重试
//指数退避：500ms->1s->2s->4s->...->最大30s
//断开后重新开始连接
//重置重试时间
//移除channel,取消监听
//销毁channel,释放channel资源
//先remove停止监听，再reset销毁对象
//最大重试延迟：30秒
//初始重试延迟：500毫秒
