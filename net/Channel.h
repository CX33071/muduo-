#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"

#include <functional>
#include <memory>
namespace mulib{
    namespace net{
        class EventLoop;
        class Channel : noncopyable{
        public:
            using EventCallback = std::function<void()>;
            using ReadEventCallback = std::function<void(base::Timestamp)>;

            Channel(EventLoop *loop, int fd);
            ~Channel(){};
            void handleEvent(base::Timestamp);
            void setReadCallback(const ReadEventCallback &cb);
            void setWriteCallback(const EventCallback &cb);
            void setErrorCallback(const EventCallback &cb);
            void setCloseCallback(const EventCallback &cb);

            int fd() const;
            int events() const;
            void set_revents(int revt);
            bool isNoneEvent() const;

            void enableReading();
            void disableReading();
            void enableWriting();
            void disableWriting();
            void disableAll();

            bool isWriting() const;
            bool isReading() const;

            int index();
            void set_index(int idx);
            EventLoop *ownerLoop();
            void setRevents(int revent) { revents_ = revent; };

        private:
            void update();
            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;
            EventLoop *loop_; // 所属的 EventLoop（1个 Channel 属于 1 个 EventLoop）
            const int fd_;    // 被监听的文件描述符
            int events_;      // 当前监听的事件（EPOLLIN / EPOLLOUT 等）
            int revents_;     // 实际发生的事件（由 Poller 设置）
            int index_;       // Poller 中使用的状态

            ReadEventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;
        };
    }
}
#endif
//

//向前声明，告诉编译器有一个类叫做EventLoop,不需要关注里面的内容，不包含头文件减少编译书监，降低头文件耦合
//回调函数类型
//一个fd对应一个EventLoop事件循环，对应一个事件控制器Channel
//有事件来时执行这个函数，函数内部再调用4个回调函数
//监听,删除读写事件
//channel的状态标记，channel是否已经添加进EventLoop
//epoll返回的就绪的事件
