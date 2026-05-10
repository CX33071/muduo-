//到了指定时间，自动执行回调函数，支持：一次性任务/循环重复任务
#pragma once
#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include <functional>
#include <atomic>
namespace muduo{
    using base::Timestamp;
    namespace net{
        class Timer:noncopyanle{
            public:
             using TimerCallback = std::function<void()>;//时间到了调用这个函数
             Timer(TimerCallback cb, Timestamp when, double interval);//when第一次触发时间，interval重复间隔，0=一次性
             void run() const;//执行回调函数
             Timestamp expiration() const;//获取下次触发时间
             bool repeat() const;//是否是重复定时器
             int64_t sequence() const;//获取定时器唯一编号
             void restart(Timestamp now);//重复定时器：重新计算下一次时间
             static int64_t numCreated();
            private:
             const TimerCallback callback_;//定时器触发时要调用的回调函数
             Timestamp expiration_;//下次触发时间
             const double interval_;//定时器的触发间隔
             const bool repeat_;//是否是周期性定时器
             const int64_t sequence_;//每创建一个Timer,这个号就会递增
             static std::atomic<int64_t> s_numCreated;//全局计数，一共多少个Timer
        }
    }
}