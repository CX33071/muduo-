//管理所有定时器，时间一到自动触发回调
#pragma once
#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "Timer.h"
#include <vector>
#include <set>
#include "Channel.h"
#include <memory>
#include "TimerId.h"
#include <atomic>
namespace muduo{
    namespace net{
    class EventLoop;
    class TimerQueue:noncopyable{//一个EventLoop对应一个定时器管理器
        public:
         TimerQueue(EventLoop* loop);
         ~TimerQueue();
         TimerId addTimer(const Timer::TimerCallback& cb,
                          Timestamp when,
                          double interval);
         void cancel(TimerId timerid);
        private:
         using Entry = std::pair<base::Timestamp, Timer*>;//时间戳+定时器指针，用来按时间排序
         using TimerList = std::set<Entry>;//自动按时间从小到大排序
         using ActiveTimer = std::pair<Timer*, int64_t>;//定时器指针+序列号，用来安全管理取消定时器，最早到期的定时器再最前面
         using ActiveTimerSet = std::set<ActiveTimer>;
         void addTimeInLoop(Timer* timer);//在IO线程内添加定时器
         void cancleInLoop(TimerId timerid);//在IO线程内取消定时器
         void handleRead();//时间到了！
         std::vector<Entry> getExpired(Timestamp now);//获取所有已经到期的定时器
         void reset(const std::vector<Entry>& expired, Timestamp now);//把重复定时器加入队列
         bool insert(Timer* timer);
         void resetTimerfd(int timerfd, Timestamp);
         EventLoop* loop_;
         const int timerfd_;//定时器文件描述符
         Channel timerfdChannel_;//监听timefd的channel
         TimerList timers_;//按时间排序的定时器队列
         ActiveTimerSet activeTimers_;//活跃定时器集合
         bool callingExpiredTimers_;//标志位，表示当卡安是否正在处理回调，避免取消时冲突
         ActiveTimerSet cancelingTimers_;//存放那些即将回调但被取消的定时器，防止回调执行
         int createTimerfd();
    };
    }  // namespace net
}