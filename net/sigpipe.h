#include <signal.h>
class IgnoreSigPipe{
    public:
     IgnoreSigPipe() { 
        ::signal(SIGPIPE, SIG_IGN);
     }
};//客户端断开连接，服务器还在向这个socket写数据，系统会向服务器发送一个信号SIGPIPE，默认收到SIGPIPE进程直接退出，所以要忽略，全局类对象程序启动自动初始化