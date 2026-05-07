//EventLoopThread是对一个线程+一个EventLoop的封装
#pragma once
#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>
namespace muduo{
    namespace net{
        class EventLoopThread{
            public:
            EventLoopThread():loop_(mullptr),exiting_(false){}
            ~EventLoopThread();
            EventLoop* startLoop();//启动线程，返回里面的loop,主线程等子线程把EventLoop创建好再返回指针
            private:
             void threadFunc();//线程真正执行的函数
             std::thread thread_;//线程对象
             EventLoop* loop_;//线程里跑的循环loop
             std::mutex mutex_;
             std::condition_variable cond_;//等待loop创建好
             bool exiting_;
        };
        }  
}
using namespace muduo::net;
inline EventLoop*EventLoopThread::startLoop(){
    thread_ = std::thread([this] { threadFunc(); });
    {//等待线程里的loop创建完成
        std::unique_lock < std::mutex lock(mutex_);
        cond_.wait(lock, [this]() { return loop_ != nullptr; });
    }
    return loop_;//返回创建好的EVentLoop
}
inline EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_!=nullptr){
        loop_->quit();//loop_创好了退出loop循环
    }
    thread_.join();//等待线程结束
}
inline void EventLoopThread::threadFunc(){
    EvnetLoop loop;//创建
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();//唤醒，通知主线程已经创好
    }
    loop.loop(-1);//启动事件循环，阻塞再这里
}