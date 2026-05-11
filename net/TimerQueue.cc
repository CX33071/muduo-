#include "TimerQueue.h"
#include <assert.h>
#include <memory.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <algorithm>
#include "../base/Timestamp.h"
#include "../base/logger.h"
#include "EventLoop.h"
#include "TimerId.h"

using namespace mulib::net;
TimerQueue::TimerQueue(EventLoop*loop):loop_(loop),timerfd_(createTimerfd()),timerfdChannel_(loop,timerfd_),timers_(){}//绑定所属事件循环、创建定时器fd、把timerfd包装成channel交给EventLoop监听事件、初始化存放定时器的有序set
int TimerQueue:createTimerfd(){
    return timerfd_create(
        CLOCK_MONOTONIC,
        TFD_NONBLOCK |
            TFD_CLOEXEC);  // CLOCK_MONOTONIC从系统启动开始计算=时不受系统时间修改影响，TFD_CLOEXEC执行exec时自动关闭fd,防止子进程继承泄露
}
TimerId TimerQueue::addTimer(const Timer::TimerCallback&cb,Timestamp when,double interval){//外部接口
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop([this, timer] { addTimerInLoop(timer); });//把添加操作扔给IO线程
    return TimerId(timer, timer->sequence());
}
void TimerQueue::cancel(TimerId timerid){//外部接口
    loop_->runInLoop([this, timerid] { cancleInLoop(timerid); });
}
bool TimerQueue::insert(Timer*timer){//插入定时器到两个集合
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());//保证两个集合元素数量永远相等
    bool earliestChanged = false;
    Timestamp when = timer->expiration();//拿到当前定时器的到期时间when
    TimerList::iterator it = timers_.begin();
    if(it==timers_.end()||when<it->first){//如果当前定时器是第一个插入的，或者它的触发时间比原来最早的还要早，那么我们需要更新timerfd的到期时间，设earliestChanged=true
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result =
            timers_.insert(Entry(when, timer));
        assert(result.second);//插入成功一定是true
        (void)result;//消除编译器变量未使用警告
    }
    {
        std::pair<TimerList::iterator, bool> result =
            activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }
        assert(timers_.size() == activeTimers_.size());
        return earliestChanged;
}
namespace muduo{
    namespace net{
        struct timespec howMuchTimeFromNow(Timestamp when){//计算距现在的时间差
            int64_t microseconds =
                when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();//得到还要等多少秒
                if(microseconds<100){
                    microseconds = 100;
                }
                timespec ts;
                ts.tv_sec = static_cast<time_t>(
                    microseconds / Timestamp::kMicroSecondsPerSecond);
                ts.tv_nsec = static_cast<long>(
                    (microseconds % Timestamp::kMicroSecondsPerSecond) * 100);
                return ts;//把微妙拆成秒+纳秒，适配timerfd_settime参数格式
        }
    }
}

void TimerQueue::resetTimerfd(int timerfd,Timestamp expiration){
    itimerspaec newValue;
    itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchFromNow(expiration);
    int ret = ::timerfd_settime(
        timerfd, 0, &newValue,
        &oldValue);  // 调用 timerfd_settime() 后，内核会把上一次设置的时间写入oldValue
    if(ret){
        LOG_SYSERR << "timerfd_settime()";
    }
}
void RimerQueue::addTimerInLoop(Timer*timer){
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged){
        resetTimerfd(timerfd_, timer->expiration());
    }
}
void TimerQueue::cancleInLoop(TimerId,timerid){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerid.timer_, timerid.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);//构造key去集合里查找
    if(it!=activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));//从时间序集合删掉
        assert(n == 1);
        (void)n;
        activeTimers_.erase(it);//从安全标识集合删掉
        delete it->first;//释放Timer对象内存
    }else if(callingExpiredTimers_){
        cancelingTimers_.insert(timer);//如果正在执行定时器回调，不能立刻删，先放入cancelingTimers_,标记待取消，等回调跑完再处理
        assert(timers_.size() == activeTimers_.size());
    }
}
void TimerQueue::handleRead(){
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    std::vector<Entry> expired = getExpired(now);//获取当前时间，取出所有已到期的定时器
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();//标记正在执行到期回调，清空待取消列表
    for(auto&it:expired){//逐个执行定时器回调函数
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);//处理重复定时器。回收一次性定时器
}
std::vector<muduo::net::TimerQueue::Entry>TimerQueue::getExpired(Timestamp now){//取出所有到期定时器
    assert(timers_.size() == activeTimers_.size());
    std::vector<muduo::net::TimerQueue::Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));//造一个哨兵：时间为当前时刻，Timer指针为极大值
    TimerList::iterator end = timers_.lower_bound(sentry);//找到第一个未到期的定时器位置，前面所有元素全是已到期
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);//把到期区间复制出来，从timers_中整体删除
    for(const Entry&it:expired){
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)n;
    }//同步从activeTimers_删掉这些到期定时器
    assert(timers_.size()==activeTimers_.size());
    return expired;
}
void TimerQueue::reset(const std::vector<Entry>&expired,Timestamp now){
    Timestamp nextExpire;
    for(auto&it:expired){
        ActiveTimer timer(it.second, it.second->sequence());//遍历每一个到期定时器
        if(it.second->repeat()&&cancelingTimers_.find(timer)==cancelingTimers_.end()){//如果是重读定时器且没被标记取消
            it.second->restart(now);//重新计算下一次触发时间
            insert(it.second);//重新插入定时器集合
        }else{//一次性定时器或已取消直接释放
            delete it.second;
        }
    }
    if(!timers_.empty()){
        nextExpire = timers_.begin()->second->expiration();
    }
    if(nextExpire.valid()){
        resetTimerfd(timerfd_, nextExpire);
    }//找到当前最早未到期定时器，重新设置内核timefd超时
}
TimerQueue::~TimerQueue(){
    ::close(timerfd_);
    for(const Entry&timer:timers_){
        delete timer.second;//释放所有定时器Timerfd对象
    }
}