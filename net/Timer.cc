#include "Timer.h"

using namespace mulib::net;
std::atomic<int64_t> Timer::s_numCreated{0};
Timer::Timer(TimerCallback cb, Timestamp when, double interval) : callback_(cb), expiration_(when), 
interval_(interval), repeat_(interval > 0),
sequence_(s_numCreated.fetch_add(1))
{}
void Timer::run() const{
    callback_();
}
mulib::base::Timestamp Timer::expiration() const{
    return expiration_;
}
bool Timer::repeat() const{
    return repeat_;
}
int64_t Timer::sequence() const{
    return sequence_;
}//
void Timer::restart(mulib::base::Timestamp now){
    if (repeat_)
    {
        expiration_ = expiration_.addTime(now, interval_);
    }
    else
    {
        expiration_ = mulib::base::Timestamp(-1);
    }
}
int64_t Timer::numCreated(){
    return s_numCreated.load();
}

//原子整型，线程安全
//回调，时间到执行任务
//获取到期时间，返回该定时器下次什么时候触发
//false一次性true周期性
//唯一ID
//新的到期时间=当前时间+间隔时间
//无效时间
//一共多少
