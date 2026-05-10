//定时期的身份证，防止删除一个已经被销毁的定时器
#pragma once
#include "Timer.h"
namespace muduo{
    namespace net{
    class Timer;
    class TimerId{
        public:
        TimerId():timer_(nullptr),sequence_(0){}
        TimerId(Timer*timer,int64_t seq):timer_(timer),sequence_(seq){}
        friend class TimerQueue;//让TimerQueue可以访问内部成员
        private:
         Timer* timer_;
         int64_t sequence_;
    };
    }  // namespace net
}