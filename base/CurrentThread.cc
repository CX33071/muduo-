#include "CurrentThread.h"
#include <sys/syscall.h>
#include <unistd.h>

namespace mulib{
    namespace CurrentThread{
        extern __thread int t_cachedTid = 0;
        pid_t tid()
        {
            if (t_cachedTid == 0)
            {
                t_cachedTid = gettid();
            }
            return t_cachedTid;
        }
        pid_t gettid()
        {
            return static_cast<pid_t>(::syscall(SYS_gettid)); // 获得线程 ID
        }
    }
}

//syscall是系统函数，直接进入函数，具体用什么系统调用取决于宏，SYS_gettid宏告诉内核我要获取当前线程的ID，返回值是long
