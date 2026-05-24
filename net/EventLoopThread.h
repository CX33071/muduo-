#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace mulib{
    namespace net{
        class EventLoopThread
        {
        public:
            EventLoopThread() : loop_(nullptr), exiting_(false) {}
            ~EventLoopThread();
            EventLoop *startLoop();

        private:
            void threadFunc();
            std::thread thread_;
            EventLoop *loop_;
            std::mutex mutex_;
            std::condition_variable cond_;
            bool exiting_;
        };
    }
}
//
using namespace mulib::net;
inline EventLoop *EventLoopThread::startLoop()
{
    thread_ = std::thread([this]
                          { threadFunc(); });

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]()
                   { return loop_ != nullptr; });
    }

    return loop_;
}
inline EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
    }
    thread_.join();
}
inline void EventLoopThread::threadFunc(){
    EventLoop loop;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(-1);
}
#endif

//EventLoopThread是对一个线程+一个EventLoop的封装
//启动线程，返回里面的loop,主线程等子线程把EventLoop创建好再返回指针
//线程真正执行的函数
//线程对象
//线程里跑的循环loop
//等待loop创建好
//等待线程里的loop创建完成
//返回创建好的EVentLoop
//loop_创好了退出loop循环
//等待线程结束
//创建
//唤醒，通知主线程已经创好
//启动事件循环，阻塞再这里
