#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include <sys/types.h>
namespace mulib{
    namespace CurrentThread{
        extern __thread int t_cachedTid;
        pid_t tid();
        pid_t gettid();
        
    }
}

#endif

//__thread是GCC用于线程局部存储的关键字，每个线程有属于自己的独立的变量t_cachedTid,用来声明线程局部存储变量，不用缓存的话每次要用线程ID都要进内核拿完ID再返回用户态，开销大
