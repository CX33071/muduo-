#include "logger.h"
using namespace muduo::base;
Logger::SourceFile::SourceFile(const char*filename):data_(filename){
    const char* slash = strrchr(filename, '/');
    if(slash){
        data_ = slash + 1;
    }
    size_ = static_cast<int>(strlen(data_));
}
Logger::Impl::Impl(LogLevel level,int savedErrno,const SourceFile&file,int line):time_(Timestamp::now()),stream_(),level_(level),line_(line),basename_(file){
    formatTime();
    formatLevel();//自动拼接时间和级别
    if(savedErrno!=0){
        stream_ << strerror(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}
void Logger::Impl::formatTime(){
    stream_ << time_.toFormattedString() << " ";
}
void Logger::Impl::formatLevel(){
    switch(level_){
        case TRACE:
            stream_ << "\033[36m" << "[TRACE] " << "\033[0m";
            break;
        case DEBUG:
            stream_ << "\033[34m" << "[DEBUG] " << "\033[0m";
            break;
        case INFO:
            stream_ << "\033[32m" << "[INFO] " << "\033[0m";
            break;
        case WARN:
            stream_ << "\033[33m" << "[WARN] " << "\033[0m";
            break;
        case ERROR:
            stream_ << "\033[31m" << "[ERROR] " << "\033[0m";
            break;
        case FATAL:
            stream_ << "\033[35m" << "[FATAL] " << "\033[0m";
            break;
        default:
            stream_ << "[UNKWN] ";
            break;
    }
}
void Logger::Impl::finish(){
    stream_ << " - " << basename_.data_ << ":" << line_ << '\n';
}
