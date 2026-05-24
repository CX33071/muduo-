#include "logStream.h"

using namespace mulib::base;
LogStream &LogStream::operator<<(const bool &val)
{
    if (isMaxString())
    {
        buffer_ += val ? "true" : "false";
    }
    return *this;
}
LogStream &LogStream::operator<<(std::ostream &(*manip)(std::ostream &))
{
    std::ostringstream oss;
    oss << manip; // 这可以捕获 std::endl
    buffer_ += oss.str();
    return *this;
}
const std::string &LogStream::str() const
{
    return buffer_;
}
void LogStream::reset()
{
    buffer_.clear();
}
void LogStream::append(const std::string &s, size_t pos, int n)
{
    if (isMaxString())
        buffer_.append(s, pos, n);
}
void LogStream::append(const std::string &s)
{
    if (isMaxString())
        buffer_ += s;
}

// 定义一个临时字符串转换器，用来把任意数据变成字符串
// 把输入的数据转为字符串存在oss里
// 为了链式调用<<,返回对象自己，指针解引用返回对象本身
