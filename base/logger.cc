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
Logger::Logger(SourceFile file,int line):impl_(INFO,0,file,line){}
Logger::Logger(SourceFile file,int line,LogLevel level,const char*func):impl_(level,0,file,line){
    impl_.stream_ << func << " ";
}
Logger::Logger(SourceFile file,int line,bool toAbort):impl_(toAbort?FATAL:ERROR,errno,file,line){}
LogStream&Logger::stream(){
    return impl_.stream_;
}
Logger::LogLevel g_logLevel = Logger::INFO;
Logger::LogLevel Logger::logLevel(){
    return g_logLevel;
}
void Logger::setLogLevel(Logger::LogLevel level){
    g_logLevel = level;
}
bool FlushFunc(){

}
Logger::~Logger(){
    impl_.finish();
    const std::string& msg = impl_.stream_.str();
    if(OutputFunc()){//如果有自定义输出到哪的函数就执行，否则输出到屏幕
        // OutputFunc(msg.c_str(),static_cast<int>(msg.size()));
    }else{
        fwrite(msg.c_str(), 1, msg.size(),stdout);
    }
    if(impl_.level_==FATAL){
        if(FlushFunc()){//自定义刷新函数，输出到文件就fflush文件
            FlushFunc();
        }else{
            fflush(stdout);
        }
        abort();
    }
}
