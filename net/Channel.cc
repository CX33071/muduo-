#include "Channel.h"
#include <poll.h>
#include "../base/logger.h"
#include "EventLoop.h"
using namespace mulib::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd) : 
loop_(loop), fd_(fd), events_(kNoneEvent), revents_(kNoneEvent), index_(-1) {}

void Channel::handleEvent(mulib::base::Timestamp receiveTime)
{
    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "Chanenl::handle_event() POLLNVAL";
    }

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}

void Channel::setReadCallback(const ReadEventCallback &cb){
    readCallback_ = cb;
}
void Channel::setWriteCallback(const EventCallback &cb){
    writeCallback_ = cb;
}
void Channel::setErrorCallback(const EventCallback &cb){
    errorCallback_ = cb;
}
void Channel::setCloseCallback(const EventCallback &cb){
    errorCallback_ = cb;
}

int Channel::fd() const{
    return fd_;
}

int Channel::events() const{
    return events_;
}
void Channel::set_revents(int revt){
    revents_ = revt;
}

bool Channel::isNoneEvent() const{
    return events_ == kNoneEvent;
}
bool Channel::isWriting() const{
    return events_ & kWriteEvent;
}
bool Channel::isReading() const{
    return events_ & kReadEvent;
}

int Channel::index(){
    return index_;
}
void Channel::set_index(int idx){
    index_ = idx;
}

EventLoop *Channel::ownerLoop(){
    return loop_;
}

void Channel::enableReading(){
    events_ |= kReadEvent;
    update();
}

void Channel::disableReading(){
    events_ &= ~kReadEvent;
}

void Channel::enableWriting(){
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting(){
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll(){
    events_ = kNoneEvent;
    update();
}
void Channel::update(){
    loop_->updateChannel(this);
}//

//描述符非法
//对端关闭
//错误
//设置回调
