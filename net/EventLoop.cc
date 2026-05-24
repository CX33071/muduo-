#include "EventLoop.h"
#include <iostream>
#include <sys/epoll.h>
#include "Channel.h"
#include "Epoller.h"
#include <sys/eventfd.h>
#include "../base/logger.h"
#include <unistd.h>
#include <assert.h>

using namespace mulib::net;

thread_local EventLoop *t_loopInThisThread = nullptr;
IgnoreSigPipe __on;
EventLoop::EventLoop()
: looping_(false), quit_(false),iteration_(0),
threadId_(std::this_thread::get_id()),
poller_(std::make_unique<Epoller>(this)),
wakeupFd_(createEventfd()),
wakeupChannel_(new Channel(this, wakeupFd_)){
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread){
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
        << " exists in this thread " << threadId_;
    }
    else{
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback([this](Timestamp)
                                    { handleRead(); });
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop(){
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_ 
    << " destructs in thread " << std::this_thread::get_id();
    wakeupChannel_->disableAll();
    t_loopInThisThread = nullptr;
}
void EventLoop::loop(int timeout = -1){
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    while(!quit_){
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(timeout, activeChannels_);
        for(auto channel : activeChannels_){
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }
    looping_ = false;
}
void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()){
        wakeup();
    }
}

bool EventLoop::isInLoopThread() const{
    return std::this_thread::get_id() == threadId_;
}

void EventLoop::assertInLoopThread(){
    if(!isInLoopThread()){
        abortNotInLoopThread();
    }
}
void EventLoop::abortNotInLoopThread(){
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << std::this_thread::get_id();
}
void EventLoop::runInLoop(const Functor &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else{
        queueInLoop(cb);
    }
}
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}
void EventLoop::queueInLoop(const Functor &cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // 加入待执行队列
    }
    // 如果不是在本线程，或者正在调用 functors（说明是嵌套），就唤醒 EventLoop
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}
//
int EventLoop::createEventfd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort(); // 或者抛出异常
    }
    return evtfd;
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
} // 读出写进 eventfd 的值，清空事件，避免 epoll 一直触发。
void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(auto functor : functors){
        functor();
    }

    callingPendingFunctors_ = false;
}

void EventLoop::updateChannel(Channel *channel){
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannel(channel);
}

TimerId EventLoop::runAt(const Timestamp &time, const Timer::TimerCallback &cb){
    return timerQueue_->addTimer(cb, time, 0);
}
TimerId EventLoop::runAfter(double delay, const Timer::TimerCallback &cb){
    
    Timestamp time(Timestamp::addTime(Timestamp::now(), delay));
    return runAt(time, cb);
} // 非静态成员函数或变量必须依附于某个具体对象
TimerId EventLoop::runEvery(double interval, const Timer::TimerCallback &cb){

    Timestamp time(Timestamp::addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}
void EventLoop::cancel(TimerId id){
    return timerQueue_->cancel(id);
}

//thread_local=每个线程独有一份，一个线程只能有一个EventLoop
//全局忽略SIGPIPE信号，防止写已关闭socket导致程序崩溃
//保存创建时的线程ID
//创建Epoller
//创建eventfd
//包装成Channel
//一个线程智能有一个EventLoop
//设置wakeupfd的读回调
//监听读事件
//取消所有事件
//线程标记清空
//整个muduo的主循环
//epoll_wait
//处理所有活跃channel的事件
//执行跨线程任务
//唤醒epoll,让它退出阻塞
//当前就是IO线程，直接执行
//丢到队列，异步执行
//发送唤醒信号
//加入待执行队列
// 如果不是在本线程，或者正在调用 functors（说明是嵌套），就唤醒 EventLoop
//创建一个事件fd,专门用来唤醒epoll
//读出写进eventfd的值，清空事件，避免epoll一直触发
//交换队列，减少锁临界区
//转发给Epoller的函数,EventLoop不做实际工作，全部交给Epoller
//定时任务，交给TimerQueue执行
//非静态成员函数或变量必须依附于某个具体对象
