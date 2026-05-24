#include "TimerQueue.h"
#include "EventLoop.h"
#include "TimerId.h"
#include <algorithm>
#include "../base/Timestamp.h"
#include "../base/logger.h"
#include <sys/timerfd.h>
#include <assert.h>
#include <unistd.h>
#include <memory.h>

using namespace mulib::net;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_() {}

int TimerQueue::createTimerfd(){
    return timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
}
TimerId TimerQueue::addTimer(const Timer::TimerCallback &cb, Timestamp when, double interval){
    Timer *timer = new Timer(cb, when, interval);
    loop_->runInLoop([this, timer]
                     { addTimerInLoop(timer); 
                    });
    return TimerId(timer,timer->sequence());
}
void TimerQueue::cancel(TimerId timerid){
    loop_->runInLoop([this, timerid]
                     { cancelInLoop(timerid); });
}

bool TimerQueue::insert(Timer *timer){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first){
        earliestChanged = true; // 如果当前定时器是第一个插入的，或者它的触发时间比原来最早的还要早，那么我们需要更新 timerfd 的到期时间（所以设 earliestChanged = true）。
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);// set只能插入没有的变量
        (void)result; // 显式声明我知道这个变量没用，但我故意写它为了避免编译器报 “变量未使用” 的警告
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}
namespace mulib{
    namespace net{
        struct timespec howMuchTimeFromNow(Timestamp when){
            int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
            if(microseconds < 100){
            microseconds = 100;
            }
            timespec ts;
            ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
            ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
            return ts;
        }
    }
}

void TimerQueue::resetTimerfd(int timerfd, Timestamp expiration)
{
    itimerspec newValue;
    itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if(ret){
        LOG_SYSERR << "timerfd_settime()";
    }
}
void TimerQueue::addTimerInLoop(Timer *timer){
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged){
        resetTimerfd(timerfd_, timer->expiration());
    }
}
void TimerQueue::cancelInLoop(TimerId timerid){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerid.timer_, timerid.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;
        
        activeTimers_.erase(it);// 反顺序
        delete it->first;
    }else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}
void TimerQueue::handleRead(){
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for(auto& it : expired){
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}
std::vector<mulib::net::TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    assert(timers_.size() == activeTimers_.size());
    std::vector<mulib::net::TimerQueue::Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);

    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now){
    Timestamp nextExpire;
    for(auto& it : expired){
        ActiveTimer timer(it.second, it.second->sequence());
        if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()){
            it.second->restart(now);
            insert(it.second);
        }
        else{
            delete it.second;
        }    
    }
    if(!timers_.empty()){
        nextExpire = timers_.begin()->second->expiration();
    }
    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}
TimerQueue::~TimerQueue(){
    ::close(timerfd_);
    for (const Entry &timer : timers_)
    {
        delete timer.second;
    }
}//

// CLOCK_MONOTONIC从系统启动开始计算=时不受系统时间修改影响，TFD_CLOEXEC执行exec时自动关闭fd,防止子进程继承泄露
//绑定所属事件循环、创建定时器fd、把timerfd包装成channel交给EventLoop监听事件、初始化存放定时器的有序set
//释放所有定时器Timerfd对象
//外部接口
//把添加操作扔给IO线程
//外部接口
//插入定时器到两个集合
// 调用 timerfd_settime() 后，内核会把上一次设置的时间写入oldValue
//构造key去集合里查找
//从时间序集合删掉
//释放Timer对象内存
//从安全标识集合删掉
//如果正在执行定时器回调，不能立刻删，先放入cancelingTimers_,标记待取消，等回调跑完再处理
//获取当前时间，取出所有已到期的定时器
//标记正在执行到期回调，清空待取消列表
//逐个执行定时器回调函数
//处理重复定时器。回收一次性定时器
//取出所有到期定时器
//造一个哨兵：时间为当前时刻，Timer指针为极大值
//找到第一个未到期的定时器位置，前面所有元素全是已到期
//把到期区间复制出来，从timers_中整体删除
//同步从activeTimers_删掉这些到期定时器
//遍历每一个到期定时器
//如果是重读定时器且没被标记取消
//重新计算下一次触发时间
//重新插入定时器集合
//一次性定时器或已取消直接释放
//找到当前最早未到期定时器，重新设置内核timefd超时
