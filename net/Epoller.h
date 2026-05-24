#ifndef MUDUO_NET_EPOLLPOLLER_H
#define MUDUO_NET_EPOLLPOLLER_H

#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include <vector>
#include <sys/epoll.h>
#include <map>

namespace mulib{
    namespace net{
        class Channel;
        class EventLoop;
        class Epoller : noncopyable{
        public:
            using ChannelList = std::vector<Channel *>;
            Epoller(EventLoop *loop);
            ~Epoller();
            base::Timestamp poll(int timeoutMs, ChannelList &activeChannels);

            void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }
            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);

        private:
            static const int kInitEventListSize = 16;
            void fillActiveChannels(int numEvents, ChannelList &activeChannels) const;
            void update(int opt, Channel *channel);

            using EventList = std::vector<struct epoll_event>;
            using ChannelMap = std::map<int, Channel *>;

            EventLoop *ownerLoop_;
            int epollfd_;
            EventList events_;
            ChannelMap channels_;
            
        };
    }
}

#endif//

//存放Channel指针的数组，用来存放活跃的Channel(有事件发生的)
//设置阻塞等待，epoll_wait返回事件发生时间戳
//确保函数在IO线程执行，确保Epoller操作必须在EventLoop所在线程
//修改Channel监听事件
//初始化事件数组大小
//epoll_wait返回时间后，把事件填到activeChannel里
//调用epoll_ctl
//内核事件数组，用来接受epoll返回的一堆事件
//fd到Channel的映射表，fd和自己的事件控制器
//属于哪个EventLoop主循环
//接收epoll_wait返回的时间
