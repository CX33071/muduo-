#include "Connector.h"
#include <assert.h>
#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketOps.h"
using namespace mulib::net;
Connector::Connector(EventLoop*loop,const InetAddress&serverAddr):loop_(loop),serverAddr_(serverAddr),connect_(false),state_(kDisconnected),retryDelayMs{//绑定事件循环、要连接的服务器地址、初始不连接、初始状态：未连接、初始重试等待500ms
    LOG_DEBUG << "ctor[" << this << "]";
}
Connector::~Connector(){
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!channel_);//确保channel已经清理
}
void Connector::start(){
    connect_ = true;
    loop_->runInLoop([this] { startInLoop(); });//扔到IO线程执行,开始尝试连接服务器
}
void Connector::startInLoop(){
    loop_->assertInLoopThread();//确保在IO线程
    assert(state_ == kDisconnected);//确保状态是未连接
    if(connect_){
        connect();//真正发起连接
    }else{
        LOG_DEBUG << "do not connect";
    }
}
void Connector::stop(){//外部停止接口
    connect_ = false;
    loop_->queueInLoop([this] { stopInLoop(); });
}
void Connector::stopInLoop(){
    loop_->assertInLoopThread();
    if(state_=kConnecting){//如果正在连接
        setState(kDisconnected);//该状态未连接
        int sockfd = removeAndResetChannel();//移除channel
        retry(sockfd);//关闭sockfd,不重试
    }
}
void Connector::connect(){
    int sockfd = socket::createNonblockingOrDie();
    int ret = socket::connect(sockfd, serverAddr_.getSockAddr());
    int saveErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) {
        case 0:
        case EINPROGRESS://非阻塞连接，正常
        case EINTR:
        case EISCONN:
            connecting(sockfd);//进入连接中
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED://连接被拒绝
        case ENETUNREACH://网络不可达
            retry(sockfd);//重试
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "connect error in Connector::startInLoop "
                       << savedErrno;
            socket::close(sockfd);//致命错误，关闭
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop "
                       << savedErrno;
            socket::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}
void COnnector::connecting(int sockfd){//进入连接中状态
    setState(kConnecting);//状态：连接中
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));  // 创建channel
    channel_->setWriteCallback([this] { Connector::handleWrite(); });//可写事件=连接完成
    channel_->serErrorCallback([this] { Connector::handleErrir(); });//错误事件
    channel_->enableWriting();//监听可写
}
void Connector::handleWrite() {
    LOG_TRACE << "Connector::handleWrite " << state_;
    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        if (err) {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " "
                     << strerror(err);
            retry(sockfd);
        } else if (socket::isSelfConnect(sockfd)) {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        } else {
            setState(kConnected);
            if (connect_) {
                newConnectionCallback_(sockfd);
            } else {
                socket::close(sockfd);
            }
        }
    } else {
        assert(state_ == kDisconnected);
    }
}
void Connector::handleError(){
    LOG_ERROR << "Connector::handleError state=" << state_;
    if(state==kConnecting){
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}
//自动重连：连接失败、关闭socket、延迟重试、等待时间翻倍
void Connector::retry(int sockfd){
    socket::close(sockfd);//关闭旧的sockfd
    setState(kDisconnected);
    if(connect_){
        LOG_INFO << "Connector::retry - Retry connecting to "
                 << serverAddr_.toHostPort() << " in " << retryDelayMs_
                 << " milliseconds. ";
        loop_->runAfter(retryDelayMs_ / 1000.0, [this] { startInLoop(); });//延迟一段时间再重试
        //指数退避：500ms->1s->2s->4s->...->最大30s
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }else{
        LOG_DEBUG << "do not connect";
    }
}
//断开后重新开始连接
void Connector::restart(){
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;//重置重试时间
    connect_ = true;
    startInLoop();
}
int Connectror::removeAndResetChannel(){//移除channel,取消监听
    channel_->disableAll();
    int sockfd = channel_->fd();
    loop_->queueInLoop([this] { resetChannel(); });
    return sockfd;
}
void Connector::resetChannel(){//销毁channel,释放channel资源
    channel_.reset();
}
//先remove停止监听，再reset销毁对象
const int Connector::kMaxRetryDelayMs;//最大重试延迟：30秒