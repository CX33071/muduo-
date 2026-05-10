#include "Timer.h"
using namespace muduo::net;
std::atomic<int64_t> Timer::s_numCreated{0};//原子整型，线程安全
Timer::Timer(TimerCallback cb,Timestamp when,double interval):callback_(cb),expiration_(when),interval_(interval),repeat_(interval>0),sequence_(s_numCreated.fetch_add(1)){}
void Timer::run()const{
    callback_();//回调，时间到执行任务
}
muduo::base::Timestamp Timer::expiration()const{//获取到期时间，返回该定时器下次什么时候触发
    return expiration_;
}
bool Timer::repeat()const{
    return repeat_;//false一次性true周期性
}
int64_t Timer::sequence()const{
    return sequence_;//唯一ID
}
void Timer::restart(muduo::base::Timestamp now){
    if(repeat_){
        expiration_ = expiration_.addTime(now, interval_);//新的到期时间=当前时间+间隔时间
    }else{
        expiration_ = Timestamp(-1);//无效时间
    }
}
int64_t Timer::numCreated(){
    return s_numCreated.load();//一共多少
}