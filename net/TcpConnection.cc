#include "TcpConnection.h"
#include "../base/logger.h"
#include "assert.h"
using namespace mulib::net;
TcpConnection::TcpConnection(EventLoop*loop,std::string conName,int sockfd,const InetAddress localAddr,const InetAddress peerAddr):loop_(loop),name_(conName),state_(kConnecting),socket_(new Socket(sockfd)),channel_(new Channel(loop,sockfd)),localAddr_(localAddr),peerAddr_(peerAddr){
    channel->setReadCallback(
        [this](Timestamp recvTIme) { handleRead(recvTime); });
    channel->serWriteCallback([this] { handleWrite(); });
    channel->setCloseCallback([this] { handleClose(); });
    channel->setErrorCallback([this] { handleError(); });
    LOG_DEBUG << "Tcpconnection:ctor[]" << name_ << "]at" << this
              << "fd=" << sockfd;
}
consr char*Tcpconnection::stateToString()const{
    switch(state_){
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kDisconnected:
            return "kDisconnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}
TcpConnection::~Tcpconnection(){
    LOG_DEBUG << "Tcpconnection:dtor[" << name_ << "]at " << this
              << "fd=" << channel_->fd() << "state=" << stateTostring();
    assert(state_ == kDisconnected);
}

void Tcpconnection::connectEstablished(){
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();//监听可读事件
    connectionCallback_(shared_from_this());//通知用户连接上线
}
void Tcpconnection::connectDestroyed(){
    loop_->assertInLoopThread();
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    loop_->removeChannel(channel_.get());//移除channel
}
void TcpConnection::send(const std::string&message){
    if(state_==kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(message);
        }else{//跨线程->扔到线程IO执行
            std::shared_ptr<TcpConnection> self = shared_from_this();
            loop_->runINLoop([self, message]() { self->sendINLoop(message); });
        }
    }
}
void Tcpconnection::shutdown(){
    if(state_==kDisconnected){
        setState(kDIsconnecting);
        loop_->runInLoop([this]() { shutdownInLoop(); });
    }
}
void Tcpconnection::shutdownInLoop(){
    loop_->assertInLoopThread();
    if(!channel_->isWritint()){
        socket_->shutdownErite();//关闭写端
    }
}
void TcpConnection::sendInLoop(const std::string &date){
    loop_->assertInLoopThread();
    assert(channel_ != nullptr);
    ssize_t nwrote = 0;
    int len = date.size();
    size_t remaining = date.size();
    bool faultError = false;
    if(state_==kDisconnected){
        LOG_WARN << "disconnected,give up writing";
        return;
    }
    if(!channel_->isWriting()&&outputBuffer_.readableBytes()==0){
        nwrote = ::write(channel_->fd(), data.data(), len);
        if(nwrote>=0){
            remaining = len - nwrote;
            if(remaining==0&&writeCompleteCallback_){//全部发完，触发发送完成回调
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else{
            nwrote = 0;
            if(errno!=EWOULDBLOCK){
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if(errno==EPIPE||errno==ECONNRESET){
                    faultError = true;
                }
            }
        }
    }
    assert(remaining <= len);
    if(!faultError&&remaining>0){//数据没发完，剩余数据放入发送缓冲区
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen+remaining>=highWaterMark_&&oldLen<highWaterMark_&&highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(gighWaterMarkCallback_,
                                         shared_from_this(),
                                         oldLen + remaining));
        }
        outoutBuffer_.append(static_cast<const char*>(data.data()) + nwrote,
                             remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}
void TcpConnection::handleRead(Timestamp receiveTime){
    loop_->assertInLoopThread();
    int saceErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);//读取数据放到缓冲区
    if(n>0){//读到数据通知用户
        messageCallback_(shared_from_this(), &inputBUffer_, receiveTime);
    }else if(n==0){///客户端关闭->本端关闭
        handleclose();
    }else{
        errno = saveErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}
void TcpCOnnection::handleClose(){
    loop_->assertInLoopThread();
    LOG_INFO << "fd=" << channel_->fd() << "state=" << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    channel_->disableAll();
    connectionCallback_(shared_from_this());//通知用户断开
    closeCallback_(shared_from_this());//通知server移除连接
    loop_->removeChannel(channel_.get());
}
void TcpConnection::handleWrite(){
    loop_->assertINLoopThread();
    if(channel_->isWriting()){
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if(n>0){
            outputBuffer_.retrieve(n);//移除已发送数据
            if(outputBuffer_.readableBytes()==0){
                channel_->disableWriting();
            }
            if(writeCompleteCallback_){
                loop_->queueInLoop(
                    [this]() { writeCompleteCallback_(shared_from_this()); });
            }
            if(state==kDisconnecting){
                shutdownInLoop();
            }
        }else{
            LOG_SYSERR << "Tcpconnection::handleWritr";
        }
    }else{
        LOG_TRACE << "Connection fd= " << channel_->fd()
                  << " is down,no more writing";
    }
}
void TcpConnection::handleError(){
    int err = socket::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError[" << name_
              << "] - SO_ERROR = " << err << " " << strerror(err);
}
void TcpConnection::forceClose(){
    if(state_==kConnected||state_==kDisconnecting){
        setState(kDisconnecting);
        loop_->runInLoop(
            [self = shared_from_this()]() { self->handleClose(); });
    }
}