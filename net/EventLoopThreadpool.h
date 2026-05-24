#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include "EventLoopThread.h"
#include "../base/noncopyable.h"

namespace mulib{
    namespace net{
        class EventLoopThreadPool : noncopyable{
        public:
            EventLoopThreadPool(EventLoop *baseloop);
            ~EventLoopThreadPool();
            void setThreadNum(int numThreads) { numThreads_ = numThreads; }
            void start();
            EventLoop *getNextLoop();

        private:
            EventLoop *baseLoop_;
            bool started_;
            int numThreads_;
            int next_;

            std::vector<std::shared_ptr<EventLoopThread>> threads_;
            std::vector<EventLoop *> loops_;
        };
        
    }
}

#endif//

//一个主线程+N个子线程
//设置线程数量，子IO线程数
//启动线程池，创建所有线程和loop
//取出下一个loop来处理新连接
//线程是否启动
//线程总数
//记录下次用第几个loop
//存放所有线程对象
//存放所有线程的loop指针
