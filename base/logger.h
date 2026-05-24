#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include "logStream.h"
#include "Timestamp.h"
#include <iostream>
#include <time.h>
#include <cstring>

#include "noncopyable.h"

#define LOG_TRACE                                          \
    if (mulib::base::Logger::logLevel() <= mulib::base::Logger::TRACE) \
    mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                          \
    if (mulib::base::Logger::logLevel() <= mulib::base::Logger::DEBUG) \
    mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                          \
    if (mulib::base::Logger::logLevel() <= mulib::base::Logger::INFO) \
    mulib::base::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::WARN).stream()
#define LOG_ERROR mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::ERROR).stream()
#define LOG_FATAL mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::FATAL).stream()
#define LOG_SYSERR mulib::base::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL mulib::base::Logger(__FILE__, __LINE__, true).stream()

namespace mulib
{
    namespace base
    {
        
        class Logger{
        public:
            enum LogLevel{
                TRACE, // 追踪程序的详细运行过程
                DEBUG, // 调试信息
                INFO,  // 一般性信息，如程序启动、配置加载成功等
                WARN,  // 警告，表示程序出现了轻微异常或潜在问题，但还能运行
                ERROR, // 错误
                FATAL  // 致命错误
            };
            class SourceFile{
            public:
                template <int N>
                SourceFile(const char (&arr)[N]); // 用于记录 __FILE__
                explicit SourceFile(const char *filename);
                const char *data_;
                int size_;
            };
            Logger(SourceFile file, int line);
            Logger(SourceFile file, int line, LogLevel level);
            Logger(SourceFile file, int line, LogLevel level, const char *func);
            Logger(SourceFile file, int line, bool toAbort); // SYSFATAL 日志
            LogStream &stream(); // 获取流式日志输入
            static LogLevel logLevel();
            void setLogLevel(LogLevel level);

            // typedef void (*OutputFunc)(const char *msg, int len);
            // typedef void (*FlushFunc)();
            // static void setOutput(OutputFunc);
            // static void setFlush(FlushFunc);

            ~Logger();

        private:
            class Impl{
            public:
                Impl(LogLevel level, int savedErrno, const SourceFile &file, int line);
                void formatTime();
                void formatLevel();
                void finish();

                mulib::base::Timestamp time_;
                LogStream stream_;
                LogLevel level_;
                int line_;
                SourceFile basename_;
            };

            Impl impl_;
        };
    }
}

using namespace mulib::base;
template <int N>
Logger::SourceFile::SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) // 用于接收固定长度数组的引用
{
    const char *slash = strrchr(data_, '/');
    if (slash)
    {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
    }
}
#endif

//\是换行连接符，告诉预处理器这句话太长了我分几行写的但是你要当成一行，#define宏默认只认一行
//最详细跟踪
//开发调试
//正常运行信息
//警告
//错误
//致命错误
//长路径文件名改为短文件名
//指向文件名指针
//接收__FILE__传过来的路径字符串，是一个固定大小的数组，这个构造函数能自动捕获数组长度N
//普通字符串指针
//作用是拼接日志，toAbort是不是致命错误，是就崩溃
//返回1日志流，让你能写<<"123"
//设置只打印什么级别以上的
//定义outputfunc这种函数的类型，参数，输出函数
//自己设置输出日志输出到哪
// 日志不是立刻输出，而是等Logger对象销毁时才输出，析构函数是日志真正打印的时候
//真正拼接日志的地方
//5个变量存了日志的全部信息
//初始化所有成员
//格式化时间
//加文件名+行号
