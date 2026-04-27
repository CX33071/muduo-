#pragma once
#include <iostream>
#include <sstream>
#include "noncopyable.h"
#define MAX_STRING_SIZE 4096//日志一行最长4096
namespace muduo{
    namespace base{
    class LogStream : noncopyable {//继承类
        private:
        bool isMaxString(){
            if(buffer_.size()<MAX_STRING_SIZE){
                return true;
            }
            std::cout << "buffer_ size reaching the maximum" << std::endl;
            return false;
        }
         std::string buffer_;
        public:
        //允许operator输出任意类型的数据
         template <typename T>
         LogStream& operator<<(const T& val);
         LogStream& operator<<(const bool& val);
         //支持std::endl这种控制输出符，std::endl是函数，必须写一个专门接受函数的<<重载
         LogStream& operator<<(std::ostream& (*manip)(std::ostream&));
         void append(const std::string& s, size_t pos, int n);
         void append(const std::string& s);
         const std::string& str() const;//获取最终拼接好的完整日志字符串
         void reset();//清空缓冲区，打印完这条日志清空准备下一条
    };
    } 
}
