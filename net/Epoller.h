#pragma once
#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include <vector>
#include <sys/epoll.h>
#include <map>
namespace muduo{
    namespace net{
    class Channel;
    class Eventloop;
    class Epoller:noncopyable{
        public:
         using ChannelList = std::vector<Channel*>;//存放Channel指针的数组，用来存放活跃的Channel(有事件发生的)
         Epoller(EventLoop*loop);
         ~Epoller();
         base::Timestamp poll(int timeoutMs,ChannelList &activeChannels);//设置阻塞等待，epoll_wait返回事件发生时间戳
         void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }//确保函数在IO线程执行，确保Epoller操作必须在EventLoop所在线程
         void updateChannel(Channel* channel);//修改Channel监听事件
         void removeChannel(Channel* channel);
         private:
          static const int kInitEventListSize = 16;//初始化事件数组大小
          void fillActiveChannels(int numEvents,ChannelList&activeChannels)const;//epoll_wait返回时间后，把事件填到activeChannel里
          void update(int opt, Channel* channel);//调用epoll_ctl
          using EventList = std::vector<struct epoll_event>;//内核事件数组，用来接受epoll返回的一堆事件
          using ChannelMap = std::map<int, Channel*>;//fd到Channel的映射表，fd和自己的事件控制器
          EventLoop* ownerLoop_;//属于哪个EventLoop主循环
          int epollfd_;
          EventList events_;//接收epoll_wait返回的时间
          ChannelMap channels_;

    };
    }  
}