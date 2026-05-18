#pragma once
#include <thread>
#include <mutex>
#include "../base/noncopyable.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include "sigpipe.h"
#include <memory>
#include <vector>
#include <atomic>
namespace muduo{
    namespace net{
    class Epoller;
    class Channel;
    class EventLoop:noncopyable{//一个线程只能有一个EventLoop，不能复制
        public:
         EventLoop();
         ~EventLoop();
         void loop(int timeout);//灵魂，事件循环的死循环
         void quit();
         void assertInLoopThread();
         bool isInLoopThread() const;
         void updateChannel(Channel* channel);
         void removeChannel(Channel* channel);
         //定时器接口
         TimerId runAt(const Timestamp& time, const Timer::TimerCallback& cb);//在指定时间执行
         TimerId runAfter(double delay, const Timer::TimerCallback& cb);//延迟多久执行
         TimerId runEvery(double interval, const Timer::TimerCallback& cb);//每隔多久执行
         void cancel(TimerId id);//取消定时器
         //跨线程执行任务
         using Functor = std::function<void()>;
         void runInLoop(const Functor& cb);
         void queueInLoop(const Functor& cb);
         //用eventfd唤醒阻塞在epoll_wait的EventLoop
         void wakeup();
         int createEventfd();
        private:
         void abortNotInLoopThread();
         void handleRead();//执行wakeupfd读事件
         void dopendingFunctors();//执行所有跨线程丢过来的任务
         using ChannelList = std::vector<Channel*>;//活跃Channel列表
         bool looping_;//是否处于loop()状态，是否已经在事件循环中
         std::atomic<bool> quit_;//原子布尔，线程安全的退出标记
         int64_t iteration_;//循环次数，调试或者统计用
         Timestamp pollReturnTime_;//每轮poll返回时间戳，用于定时器判断
         const std::thread::id threadId_;//创建该EventLoop的线程id,用于检查当前函数是不是在正确线程执行
         std::unique_ptr<Epoller> poller_;//epoll封装器
         ChannelList activeChannel_;//epoll返回的活跃Channel
         //跨线程任务队列+锁
         std::vector<Functor> pendingFunctors_;//延迟执行的任务队列
         bool callingPendingFunctors_;//是否正在执行pendingFunctors_,防止嵌套调用
         std::mutex mutex_;//保护pendingFunctors_的互斥锁
         int wakeupFd_;
         std::unique_ptr<Channel> wakeupChannel_;
         std::unique_ptr<TimerQueue> timerQueue_;//定时器队列
         //unique_ptr智能指针，自动delete,专属的对象，别人不能复制，不能共享
    };
    }  
}
//业务线程调用runInLoop时，如果已经在IO线程，epoll已经被唤醒，就直接执行任务，但是如果不在IO线程，此时要进入IO线程执行任务，先把任务添加进队列，业务线程已经被唤醒，但是IO线程此刻不一定被唤醒，所以要用wakeupfd唤醒IO线程，让IO线程知道队列来任务了，去执行,之后再用handleRead取出wakeup()写进去的数据，因为wakeupfd是水平触发，不取出来一直通知有可读事件，真正处理读事件用的是Tcpconnection的handleRead()