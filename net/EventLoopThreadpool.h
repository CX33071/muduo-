//一个主线程+N个子线程
#pragma once
#include "EventLoopThread.h"
#include "../base/noncopyable.h"
namespace muduo{
    namespace net{
        class EventLoopThreadPool:noncopyable{
            public:
             EventLoopThreadPool(EventLoop* baseloop);
             ~EventLoopThreadPool();
             void setThreadNum(int numThreads) { numTHreads_ = numThreads; }//设置线程数量，子IO线程数
             void start();//启动线程池，创建所有线程和loop
             EventLoop* getNextLoop();//取出下一个loop来处理新连接
            private:
             EventLoop* baseLoop_;
             bool started_;//线程是否启动
             int numThreads_;//线程总数
             int next_;//记录下次用第几个loop
             std::vector<std::shared_ptr<EventLoopThread>> threads_;//存放所有线程对象
             std::vector<EventLoop*> loops_;//存放所有线程的loop指针
        };
        }  
}