//TCP客户端核心
#pragma once
#include "../base/noncopyable.h"
#include "InetAddress.h"
#include <functional>
#include <memory>
namespace muduo{
    namespace net{
    class Channel;
    class EventLoop;
    class Connector:noncopyable,public std::enable_shared_from_this<Connector>{//安全使用shared_ptr管理自己
        public:
         using NewConnectionCallback = std::function<void(int sockfd)>;
         Connector(EventLoop* loop, const InetAddress& serverAddr);//客户端事件循环、要连接的服务器地址
         ~Connector();
         void setNewConnectionCallback(const NewConnectionCallback& cb){
             newConnectionCallback_ = cb;
         }
         void start();//开始连接
         void restart();//重连
         void stop();
         const InetAddress& serverAddress() const { return serverAddr_; }
         private:
          enum States { kDisconnected, //未连接
            kConnecting,//连接中 
            kConnected//已连接
             };//用状态及保证不会重复连接，不会乱序
          static const int kMaxRetryDelayMs = 30 * 1000;//最大重试间隔30s
          static const int kInitRetryDelayMs = 500;//初始间隔500ms
          void setState(States s) { state_ = s; }//设置状态
          void startInLoop();//真正开始连接，在IO线程执行,外部调用start(),转到startInLoop()
          void stopInLoop();//真正停止连接,外部调用stop
          void connect();//发起连接
          void connecting(int sockfd);//连接中处理
          void handleWrite();//连接成功，可写事件触发
          void handleError();//连接出错
          void retry(int sockfd);//连接失败，重试
          int removeAndResetChannel();//移除并重置channel
          void resetChannel();//重置channel
          EventLoop* loop_;//客户端所在事件循环
          InetAddress serverAddr_;//服务器地址
          bool connect_;//是否正在连接
          States state_;//连接状态
          std::unique_ptr<Channel> channel_;//指向客户端的channel
          NewConnectionCallback newConnectionCallback_;//连接成功后的回调
          int retryDelayMs_;//重试的延迟时间
    };
    }  // namespace net
}