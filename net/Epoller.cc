#include "Epoller.h"
#include <errno.h>
#include <unistd.h>

const int _knew = -1; // Channel 从没添加进 poller
const int _kadded = 1; // 已添加
const int _kdelete = 2; // 曾经添加过，现在删除了
using namespace mulib::net;
using ChannelList = std::vector<Channel *>;
Epoller::Epoller(EventLoop *loop) :
ownerLoop_(loop),epollfd_(epoll_create1(EPOLL_CLOEXEC)),events_(kInitEventListSize){
    if (epollfd_ < 0)
    {
        LOG_FATAL << "Epoller::Epoller";
    }
}

Epoller::~Epoller()
{
    close(epollfd_);
}

mulib::base::Timestamp Epoller::poll(int timeoutMs, ChannelList &activeChannels){
    Timestamp now(Timestamp::now());
    while(1){
        int numEvents = epoll_wait(epollfd_, events_.data(), events_.size(), timeoutMs);
        now = Timestamp::now();
    if(numEvents > 0){
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()){
            events_.resize(2 * events_.size());
        }
        break;
    }
    else if(numEvents == 0){
        LOG_INFO << "Epoller::epoll: nothing happend!";
        break;
    }
    else{
        if (errno == EINTR) {
            continue;  // 被信号打断，重试
        }
        LOG_WARN << "Epoller::epoll: ret < 0" << strerror(errno);
        break;
    }
    }
    
    return now;
}
void Epoller::fillActiveChannels(int numEvents, ChannelList &activeChannels) const{
    for (int i = 0; i < numEvents;i++){
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels.push_back(channel);
    }
}
void Epoller::updateChannel(Channel *channel){
    assertInLoopThread();
    int index = channel->index();
    int fd = channel->fd();
    if (index == _knew || index == _kdelete){
        if (index == _knew)
        {
            channels_[fd] = channel;
        }
        channel->set_index(_kadded);
        update(EPOLL_CTL_ADD, channel);
    }
    else{
        if (channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(_kdelete);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::removeChannel(Channel *channel)
{
    assertInLoopThread();
    int fd = channel->fd();
    int index = channel->index();
    channels_.erase(fd);
    if (index == _kadded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(_knew);
}
void Epoller::update(int opt, Channel *channel){
    ::epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, opt, fd, &event) < 0){
        if (opt == EPOLL_CTL_DEL){
            LOG_ERROR << "Epoller::epoll : delete error";
        }
        else{
            LOG_ERROR << "Epoller::epoll : add / mod error" << strerror(errno);
        }
    }
}//

//给updateChannel判断执行ADD还是DEL
//Channel从没加入epoll
//已在epoll中
//从epoll中删除
//epoll_wait内核返回活跃fd交给fillActiveChannels
//被信号打断，重试
//从内核拿到就绪fd
//加入活跃列表
//获取状态
//channel是新的或者已经删除，执行ADD
//将channel加入map进行管理
//设置状态为已添加
//channel已经存在但是没有监听任何事件，删除
//修改监听事件
//从map中删除
//从epoll中删除
