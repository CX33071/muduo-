#pragma once
#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include <functional>
#include <memory>
namespace muduo{
    namespace net{
    class EventLoop;//向前声明，告诉编译器有一个类叫做EventLoop,不需要关注里面的内容，不包含头文件减少编译书监，降低头文件耦合
    class Channel:noncopyable{
        //回调函数类型
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(base::Timestamp)>;
        Channel(EventLoop* loop, int fd);//一个fd对应一个EventLoop事件循环，对应一个事件控制器Channel
        ~Channel() {};
        void handleEvent(base::Timestamp);//有事件来时执行这个函数，函数内部再调用4个回调函数
        void setReadCallback(const ReadEventCallback & cb);
        void setWriteCallback(const ReadEventCallback& cb);
        void setErrorCallback(const ReadEventCallback& cb);
        void setCloseCallback(const EventCallback& cb);
        int fd() const;
        int events() const;
        void set_revents(int revt);
        bool isNoneEvent() const;
        //监听,删除读写事件
        void enableReading();
        void enableWriting();
        void disableReading();
        void disableWriting();
        void disableAll();
        bool isWriting() const;
        bool isReading() const;
        int index();//channel的状态标记，channel是否已经添加进EventLoop
        void set_index(int idx);
        EventLoop* ownerLoop();
        void serRevents(int revent) { revents_ = revent; };
        private:
         void update();
         static const int kNoneEvent;
         static const int kReadEvent;
         static const int kWriteEvent;
         EventLoop* loop_;
         const int fd_;
         int events_;
         int revents_;//epoll返回的就绪的事件
         int index_;
         ReadEventCallback readCallback_;
         ReadEventCallback writeCallback_;
         EventCallback closeCallback_;
         EventCallback errorCallback_;
    };
    }  
}