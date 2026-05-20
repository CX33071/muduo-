# <center>`muduo`网络库</center>

[toc]



`muduo`是一个专为`Linux`设计的高并发TCP网络编程库

## 核心模型：

### <span style="color:#87CEEB">主从`Reactor`:</span>

主`Reactor`:`Acceptor`监听新的客户端连接

从`Reactor`:每个线程一个`EventLoop`处理已连接客户端的收发数据

### <span style="color:#87CEEB">`one loop per thread`:</span>

一个线程一个`EventLoop`，IO操作全在本线程，无锁高并发

### <span style="color:#87CEEB">事件驱动+回调编程</span>

不阻塞、不轮询，`epoll` 事件触发，业务只写回调

### <span style="color:#87CEEB">分层隔离 + 生命周期托管</span>

分层：base 底层工具 + net 网络框架，我们先来讲`net`网络框架部分

用 `shared_ptr` 管理 `TcpConnection`，线程安全析构不野指针

## 核心组件：

### `EventLoop`类

一个线程一个`EventLoop`事件循环，等待事件，处理事件，继续等待事件，每个客户端对应一个`EventLoop`，但是一个`EventLoop`对应多个客户端，一个`EventLoop`有一个`epoller`(`epoll`的封装)和多个`channel`(`fd`的管理器)，这个事件循环里进行的就是`epoll_wait`监听等待事件，处理事件，然后继续等待事件

### `Channel`类

每个`fd`对应的管理器，实现对`fd`的监听、删除事件和处理，当有`fd`就绪时调用成员函数`handleEvent`,`handleEvent`内部调用4个回调来处理读写、关闭、错误事件，是调用真正处理事件的工具的地方

### `TcpServer`类

这就是整个服务端的一个大框架，在一个主`EventLoop`循环里初始化(创建、绑定)服务端，创建`Acceptor`接受者在主`Reactor`里来接受新客户端的连接，当有新客户端连接成功时调用成员函数`newConnection`给新客户端`fd`从线程池中分配一个线程并且创建`Tcpconnection`通道，主要业务就是主`Reactor`

### `TcpClient`类

这是对应客户端的工具，和`TcpServer`一样，先创建启动客户端，每个客户端有自己的`Connector`管理者，`Connector`进行服务端的连接，连接成功后调用成员函数`newConnection`创建`Tcpconnection`通道，接下来就是客户端的收发数据业务

### `Acceptor`类

这也是服务端最核心的一个工具：接受器。顾名思义作用就是来接受新的客户端的连接，当有客户端连接时调用成员函数`handleRead`来处理，连接成功后用回调函数`newConnectionCallback_`把`sock`交给`TcpServer`创建`Tcpconnection`来管理这个连接

### `Connector`类

对应的这就是客户端的一个核心工具：连接器。用来连接服务端，有连接失败的重连和断开连接的重连函数，一样的如果成功连接上服务端有`Callback_`回调函数来通知`TcpClient`创建`Tcpconnection`通道

### `Tcpconnection`类

这就是服务端和客户端连接成功时双方都要创建的`Tcpconnection`通道，双方传数据是在这个通道里，自然发送接受数据的函数也是`Tcpconnection`类里的成员函数，`handleRead`用来读取对方发过来的数据，`send`用来发送数据，如果一次性发不完会把数据存在`outputBuffer`临时缓冲区，`handleWrite`用来自动发没发完存在`outputBuffer`的数据

### `Buffer`类

缓冲区类是收发数据的关键工具，自动扩容的`TCP`缓冲区，能够解决拆包、粘包、读写数据的任务，底层是通过`vector`和双指针(读写指针)实现的，粘包拆包问题是通过头部预留空间(收发数据长度)实现的，服务端和客户端之间通信通过检查`\r\n`基本不会出现粘包拆包问题，文件收发时需要解决粘包拆包问题

## 具体类接口

### 模块一：`Reactor`核心(事件驱动)

这个板块包括`Channel`类、`Epoller`类、`EventLoop`类、`EventLoopThread`类、`EventLoopThreadLoop`类

**`Channel.h`**

决定监听什么事件、决定事件来了调用什么函数，只是决定信息，真正监听是在`epoller`

流程：

创建一个`Channel`，绑定`fd`+`EventLoop`，设置回调`setReadCallback()`，调用`enableReading()`告诉`epoll`监听读，`epoll`等待事件，事件来了`handleEvent`被调用，调用回调函数

```c
namespace muduo{
    namespace net{
    class EventLoop;
    class Channel:noncopyable{
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(base::Timestamp)>;
        Channel(EventLoop* loop, int fd);//一个fd对应一个EventLoop事件循环，对应一个事件控制器Channel
        ~Channel() {};
        void handleEvent(base::Timestamp);//有事件来时执行这个函数，函数内部再调用4个回调函数
        void setReadCallback(const ReadEventCallback & cb);
        void setWriteCallback(const EventCallback& cb);
        void setErrorCallback(const EventCallback& cb);
        void setCloseCallback(const EventCallback& cb);
        void set_revents(int revt);
        //监听,删除读写事件
        void enableReading();
        void enableWriting();
        void disableReading();
        void disableWriting();
        void disableAll();
        int index();//channel的状态标记，channel是否已经添加进EventLoop
        void set_index(int idx);
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
         EventCallback writeCallback_;
         EventCallback closeCallback_;
         EventCallback errorCallback_;
    };
    }  
}
```

**`epoller.h`**

对`Linux epoll`的C++封装

管理`epoll`文件描述符、注册/修改/删除`Channel`、等待事件发生返回活跃的`Channel`，`EventLoop`持有`epoller`，`epoller`不和业务打交道，只做内核事件监听

流程：

创建`epollfd`，`updateChannel`把`Channel`加入`epoll`监听，`poll`调用`epoll_wait`阻塞等待，内核返回活跃事件，`fillActiveChannels`找到活跃`Channel`返回给`EventLoop`调用`Channel::handleEvent`

```c
namespace mulib{
    namespace net{
        class Channel;
        class EventLoop;
        class Epoller : noncopyable{
        public:
            using ChannelList = std::vector<Channel *>;
            Epoller(EventLoop *loop);
            ~Epoller();
            base::Timestamp poll(int timeoutMs, ChannelList &activeChannels);//调用epoll_wait阻塞在这里等待事件，输出activeChannels发生事件的channel列表，阻塞有超时限制
            void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }
            void updateChannel(Channel *channel);//把channel注册到epoll或修改监听事件，有标志位给它来判断该执行ADD/MOD/DEL
            void removeChannel(Channel *channel);
        private:
            static const int kInitEventListSize = 16;
            void fillActiveChannels(int numEvents, ChannelList &activeChannels) const;//从内核拿到返回的epoll_events，转成channel*，加入activechannels活跃列表
            void update(int opt, Channel *channel);//调用epoll_ctl的地方，这个函数供updatechannel用
            using EventList = std::vector<struct epoll_event>;//接收内核返回的事件
            using ChannelMap = std::map<int, Channel *>;

            EventLoop *ownerLoop_;
            int epollfd_;
            EventList events_;//内核返回的事件列表
            ChannelMap channels_;//fd->channel的映射表，记录所有被监听的channel
            
        };
    }
}
```

**`EventLoop.h`**

无限事件循环，等待事件，处理事件，继续等待事件，所有操作必须在创建`EventLoop`的线程执行

线程池有几个线程就有几个`EventLoop`循环，一个线程用来进行主`Reactor`，剩下的线程用来监听已经连接客户端，当有客户端连接成功就把`fd`放进任一一个线程`eventloop`里被监听，一个`eventloop`同时监听多个客户端`fd`

流程：

启动`loop`，调用`poller_->poll()`阻塞等待事件，内核返回活跃`channel`，调用`channel->handleEvent`，如果是客户端读事件，`handleEvent`调用的回调函数是`Tcpconnection`的`handleRead`，处理定时器，执行跨线程任务，回到循环继续等待

```c
thread_local EventLoop* t_loopInThisThread = nullptr;//thread_local=每个线程独有一份，一个线程只能有一个EventLoop，线程局部存储，C的话在全局或静态变量的声明中包含__thread说明符即可
IgnoreSigPipe __on;//全局忽略SIGPIPE信号，防止写已关闭(客户端关闭)socket导致程序崩溃
```

```c
namespace mulib{
    namespace net{
        class Epoller;
        class Channel;
        class EventLoop : noncopyable{
        public:
            EventLoop();
            ~EventLoop();
            void loop(int timeout);//事件循环的死循环,epoll_wait+处理活跃channel+执行定时任务+执行跨线程任务
            void quit();
            void assertInLoopThread();
            bool isInLoopThread() const;
            //channel想注册/修改/删除，调用,转发给epoller
            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);
//定时器接口，在指定时间执行，延迟多久执行
            TimerId runAt(const Timestamp &time, const Timer::TimerCallback &cb);
            TimerId runAfter(double delay, const Timer::TimerCallback &cb);
            TimerId runEvery(double interval, const Timer::TimerCallback &cb);
            void cancel(TimerId id);

            using Functor = std::function<void()>;
            void runInLoop(const Functor &cb);
            void queueInLoop(const Functor &cb);//跨线程任务扔进队列
            void wakeup();//用eventfd唤醒阻塞在epoll_wait的eventloop，异步唤醒IO线程
            int createEventfd();

        private:
            void abortNotInLoopThread();
            void handleRead();//处理wakeupfd读事件，水平触发，要拿出数据
            void doPendingFunctors();//执行所有跨线程丢过来的任务
            using ChannelList = std::vector<Channel *>;//活跃channel列表

            bool looping_;          // 是否处于 loop() 状态（是否已经在事件循环中）
            std::atomic<bool> quit_; // 是否退出循环，线程安全
            int64_t iteration_;      // 循环次数，调试或统计用
            Timestamp pollReturnTime_; // 每轮 poll 返回时间戳，用于定时器判断等
            const std::thread::id threadId_; // 创建该 EventLoop 的线程 id，用于线程检查

            std::unique_ptr<Epoller> poller_;
            ChannelList activeChannels_; // 本轮 epoll 触发的 Channel 列表

            // std::unique_ptr<TimerQueue> timerQueue_;
            //管理定时器的类（内部使用 timerfd + 最小堆）
            std::vector<Functor> pendingFunctors_; // 延迟执行的任务队列
            bool callingPendingFunctors_;          // 是否正在执行 pendingFunctors_，防止嵌套调用
            std::mutex mutex_;                     // 保护 pendingFunctors_ 的互斥锁

            int wakeupFd_;
            std::unique_ptr<Channel> wakeupChannel_;
            std::unique_ptr<TimerQueue> timerQueue_;
        };
    }
}
```

```c
void EventLoop::runInLoop(const Functor &cb){
    if(isInLoopThread()){//当前就是IO线程，直接执行
        cb();
    }else{
        queueInLoop(cb);//跨线程任务，扔到对应IO线程队列执行
    }
}
```

什么时候执行跨线程任务？

1.主线程接受新的客户端连接，现在要把客户端`fd`分配给子线程进行监听

2.例如`FTP`的服务端和客户端之间的通话，服务端给客户端发消息跳到对应IO线程去执行`send`，非IO线程调用`send`内部自动跨线程

3.服务端要求关掉某个客户端的连接

怎么执行跨线程任务？

当非IO线程把任务交给IO线程时这时只是IO线程的任务队列进来一个函数，但是IO线程并不知道有任务来了，所以需要被主动唤醒，`EventLoop`的`wakeup`函数就是用来唤醒IO线程的，`EventLoop`的`handleRead`用来取走`wakeup`写进IO线程缓冲区的东西，水平触发，防止IO线程一直被唤醒

业务线程调用`runInLoop`时，如果已经在`IO`线程，`epoll`已经被唤醒，就直接执行任务，但是如果不在`IO`线程，此时要进入`IO`线程执行任务，先把任务添加进队列，业务线程已经被唤醒，但是`IO`线程此刻不一定被唤醒，所以要用`wakeupfd`唤醒`IO`线程，让`IO`线程知道队列来任务了，去执行,之后再用`handleRead`取出`wakeup()`写进去的数据，因为`wakeupfd`是水平触发，不取出来一直通知有可读事件，真正处理读事件用的是`Tcpconnection`的`handleRead()`

**`EventLoopThread`**

是`one loop per thread`的封装，对一个线程和`EventLoop`的封装，主线程调用`startLoop`，取出一个子线程，在子线程里建一个`Loop`，建完返回loop*告诉主线程建完了，子线程`Loop.Loop()`开始对客户端的监听

```c
namespace muduo{
    namespace net{
        class EventLoopThread{
            public:
         EventLoopThread():loop_(nullptr),exiting_(false){}
            ~EventLoopThread();
            EventLoop* startLoop();//启动线程，返回里面的loop,主线程等子线程把EventLoop创建好再返回指针
            private:
             void threadFunc();//线程真正执行的函数
             std::thread thread_;//线程对象
             EventLoop* loop_;//线程里跑的循环loop
             std::mutex mutex_;
             std::condition_variable cond_;//等待loop创建好
             bool exiting_;
        };
        }  
}
using namespace muduo::net;
inline EventLoop*EventLoopThread::startLoop(){
    thread_ = std::thread([this] { threadFunc(); });
    {//等待线程里的loop创建完成
        std::unique_lock < std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() { return loop_ != nullptr; });
    }
    return loop_;//返回创建好的EVentLoop
}
inline void EventLoopThread::threadFunc(){
    EventLoop loop;//创建
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();//唤醒，通知主线程已经创好
    }
    loop.loop(-1);//启动事件循环，阻塞再这里
}
```

**`EventLoopThreadPool`**

事件循环线程池，管理主`EventLoopThread`和一堆子`EventLoopThread`，`start`启动线程池时创建所有`EventLoopThread`

```c
namespace muduo{
    namespace net{
        class EventLoopThreadPool:noncopyable{
            public:
             EventLoopThreadPool(EventLoop* baseloop);
             ~EventLoopThreadPool();
             void start();//启动线程池，创建所有线程和loop
             EventLoop* getNextLoop();//取出下一个loop来处理新连接，Acceptor连接成功时分配一个子线程
            private:
             EventLoop* baseLoop_;
             bool started_;//线程是否启动
             int numThreads_;//线程总数
             int next_;//记录下次用第几个loop
             std::vector<std::shared_ptr<EventLoopThread>> threads_;//存放所有线程对象
             std::vector<EventLoop*> loops_;//存放所有线程的loop指针
        };
        }  
}
```

#### **总结**

不实现具体业务，只搭建事件驱动+多线程调度、IO 监听框架

**流程：**

客户端发起连接→主线程 `Acceptor` 获取客户端 `fd`→线程池分配子线程` EventLoop`→创建绑定 `fd` 的  `Channel`→子线程更新通道加入 `epoll` 监听→数据抵达触发 `IO` 事件→`Epoller` 上报活跃 `Channel`→`Channel ` 执行读写回调完成数据交

### 模块二：网络基础(`socket`、地址、缓冲区)

**`InetAddress`**就是对 `sockaddr_in` 的 C++ 包装

**`SocketOps`**纯静态函数，包装`Linux`的`socketAPI`，系统调用

**`Socket`**对`socket`的对象包装，全用`Socketops`的函数

**`Buffer`**

自动扩容的`TCP`缓冲区，能够解决拆包、粘包、读写数据的任务，底层是通过`vector`和双指针(读写指针)实现，`readIndex`下次从哪读，`writeIndex`下次从哪写，数据永远在这两个指针中间

要发送的数据都先放进`outputBuffer`，最后由`EventLoop`线程统一发送

```c
namespace muduo{
    namespace net{
        class Buffer{
            public:
             const static size_t kCheapPrepend = 8;//头部预留8字节,用来在数据包前加长度，解除粘包
             const static size_t kInitialSize = 1024;//初始大小1k
             Buffer();
             size_t readableBytes() const;//可读数据长度
             size_t writeableBytes() const;//可写空间长度
             size_t prependableBytes() const;//头部预留空间
// readableBytes() = writerIndex_ - readerIndex_ writableBytes() =buffer.size() - writerIndex_
// prependableBytes() = readerIndex_
             void swap(Buffer& rhs);//交换两个缓冲区
             const char* peek() const;//获取读指针，返回第一个可读字节的指针begin()+readerIndex_
             //只移动指针，不删除数据
             void retrieve(size_t len);//读了len字节，指针往后挪len
             std::string retrieveAllAsString();//取出数据，返回字符串。自动移动读指针
             void append(const char* data, size_t len);//往缓冲区写数据，空间不足则自动扩容
             void prepend(const void* data, size_t len);//头部插入数据的长度，解决粘包
             ssize_t readFd(int, int* saveErrno);//从socket直接读到缓冲区，非阻塞IO必须用这个，一次性读尽可能多的数据
             private:
              void makeSpace(size_t len);//扩容
              std::vector<char> buffer_;
              size_t readerIndex_;//读指针
              size_t writerIndex_;//写指针
        };
        }  
}
```

**注意这里还有一个类`sigpipe`，封装了忽略信号`SIGPIPE`的逻辑，客户端断开连接，服务器还在向这个socket写数据，系统会向服务器发送一个信号`SIGPIPE`，默认收到`SIGPIPE`进程直接退出，所以要忽略，全局类对象程序启动自动初始化

#### **总结**

就是对`addr`和`socket`的封装工具类

### 模块三：`TCP`业务层(`server`、`client`连接管理)

**`Acceptor`**

创建`server socket`，有客户端来，接收连接，调用回调函数，把`sock`交给`Tcpserver`,`tcpserver`会创建`tcpconnection`管理这个连接，具体执行连接的工具

**`Connector`**

和`Acceptor`一样，创建`socket`，进行连接，有连接成功时调用回调函数，把`sock`交给`Tcpserver`,`tcpserver`会创建`tcpconnection`管理这个连接， 如果连接失败 ，自动重试（指数退避，`500ms → 1s → 2s → 4s…`)，具体执行连接的工具

这里有两个函数需要区分一下：

```c
int Connector::removeAndResetChannel(){
    channel_->disableAll();      // 第一步：停止所有事件监听
    int sockfd = channel_->fd(); // 保存 fd，后面要返回
    loop_->queueInLoop([this] {
        resetChannel();          // 第二步：把 reset 扔到队列稍后执行
    });
    return sockfd;               // 返回 fd 给调用方
}

```

```c
void Connector::resetChannel(){
    channel_.reset(); // 释放 Channel 对象
}
```

`removeAndResetChannel`停止` Channel` 上所有的读写事件监听，让 `Epoll` 不再管它，把`resetChannel`先放在队列里不立刻销毁`channel`，因为现在正在执行`channel`的回调函数，等回调执行完再销毁

**`Tcpconnection`**

**`TcpServer`**

**`TcpClient`**





