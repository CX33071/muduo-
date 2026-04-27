#pragma once
#include "logStream.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <iostream>
#include <time.h>
#include <cstring>
//\是换行连接符，告诉预处理器这句话太长了我分几行写的但是你要当成一行，#define宏默认只认一行
#define LOG_TRACE                                                            \
    if (muduo::base::Logger::logLevel() <= muduo::base::Logger::TRACE)       \
        \muduo::base::Logger(__FILE__, __LINE__, muduo::base::Logger::TRACE, \
                             __func__)                                       \
            .stream();//Logger()创建临时Logger对象，__FILE__编译器自动填当前文件名，__LINE__编译器自动填当前行号，__func__编译器自动填当前函数名，.stream()返回LogStream&,所以LOG_TRACE<<才能输出
#define LOG_DEBUG                                                       \
    if (muduo::base::Logger::logLevel() <= muduo::base::Logger::DEBUG)  \
    muduo::base::Logger(__FILE__, __LINE__, muduo::base::Logger::DEBUG, \
                        __func__)                                       \
        .stream()
#define LOG_INFO                                                       \
    if (muduo::base::Logger::logLevel() <= muduo::base::Logger::INFO)  \
    muduo::base::Logger(__FILE__, __LINE__)                                      \
.stream()
#define LOG_WARN \
    muduo::base::Logger(__FILE__, __LINE__, muduo::base::Logger::WARN).stream()
#define LOG_ERROR                                                        \
    muduo::base::Logger(__FILE__, __ILINE__, muduo::base::Logger::ERROR) \
        .stream()
#define LOG_FATAL \
    muduo::base::Logger(__FILE__, __LINE__, muduo::base::Logger::FATAL).stream()
#define LOG_SYSERR muduo::base::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::base::Logger(__FILE__,__LINE__,true).stream()
namespace muduo{
    namespace base{
    class Logger {
        public:
         enum LogLevel { 
            TRACE, //最详细跟踪
            DEBUG, //开发调试
            INFO, //正常运行信息
            WARN, //警告
            ERROR, //错误
            FATAL //致命错误
        };
        //长路径文件名改为短文件名
        class SourceFile{
            public:
             const char* data_;//指向文件名指针
             int size_;
             template <int N>
             SourceFile(const char (&arr)[N]);//接收__FILE__传过来的路径字符串，是一个固定大小的数组，这个构造函数能自动捕获数组长度N
             explicit SourceFile(const char* filename);//普通字符串指针
        };
        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char* func);
        Logger(SourceFile file, int line, bool toAbort);//作用是拼接日志，toAbort是不是致命错误，是就崩溃
        LogStream& stream();//返回1日志流，让你能写<<"123"
        static LogLevel logLevel();
        void setLogLevel(LogLevel level);//设置只打印什么级别以上的
        typedef void (*OutputFunc)(const char* msg, int len);//定义outputfunc这种函数的类型，参数，输出函数
        static void setOutput(OutputFunc);//自己设置输出日志输出到哪
        ~Logger();  // 日志不是立刻输出，而是等Logger对象销毁时才输出，析构函数是日志真正打印的时候
        private:
        class Impl{//真正拼接日志的地方
            public:
            //5个变量存了日志的全部信息
             muduo::base::Timestamp time_;
             LogStream stream_;
             LogLevel level_;
             int line_;
             SourceFile basename_;
             Impl(LogLevel level,
                  int savedErrno,
                  const SourceFile& file,
                  int line);//初始化所有成员
             void formatTime();//格式化时间
             void formatLevel();
             void finish();//加文件名+行号
        };
        Impl impl_;
    };
    } 
}
using namespace muduo::base;
template <int N>
Logger::SourceFile::SourceFile(const char (&arr)[N]):data_(arr),size_(N-1){
    const char* slash = strchr(data_, '/');
    if(slash){
        data = slash + 1;
        size -= static_cast<int>(data_ - arr);
    }
}
