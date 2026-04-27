#include "Timestamp.h"
using namespace muduo::base;
Timestamp Timestamp::now(){
    struct timeval tv;
    gettimeofday(&tv, NULL);//系统调用获取时间
    int64_t seconds = tv.tv_sec;//秒数
    return Timestamp(seconds * 1000000 + tv.tv_usec);//转成微秒生成Timestamp对象
}
void Timestamp::swap(Timestamp&that){
    std::swap(that.microSecondsSinceEpoch_, this->microSecondsSinceEpoch_);//交换传进来的时间和现在“我”的时间
}
bool Timestamp::valid()const{
    return microSecondsSinceEpoch_ > 0;
}
int64_t Timestamp::microSecondsSinceEpoch()const{
    return this->microSecondsSinceEpoch_;
}
time_t Timestamp::secondsSinceEpoch()const{
    return static_cast<time_t>(microSecondsSinceEpoch_ /
                               kMicroSecondsPerSecond);
}
std::string Timestamp::toFormattedString(bool showMicroseconds)const{
    if(!showMicroseconds){
        return "0";
    }
    time_t seconds = secondsSinceEpoch();
    tm* localTime = localtime(&seconds);//数字时间转换成日历时间，存在秒、分、时、日、月、年的结构体里
    char buf[100];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localTime);
    std::string timeString = std::to_string(this->microSecondsSinceEpoch_);
    timeString = timeString.substr(timeString.size() - 1);
    std::string FormattedString(buf);
    return FormattedString + "." + timeString;
}
Timestamp Timestamp::addTime(Timestamp timestamp,double seconds){
    int64_t delta = static_cast<int64_t>(seconds * kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
double Timestamp::timeDifference(Timestamp high,Timestamp low){
    int64_t timedifference =
        high.microSecondsSinceEpoch_ - low.microSecondsSinceEpoch_;
    double delta = static_cast<double>(timedifference / kMicroSecondsPerSecond);
    return delta;
}
bool Timestamp::operator==(const Timestamp that)const{
    return this->microSecondsSinceEpoch_ == that.microSecondsSinceEpoch_;
}
bool Timestamp::operator<(const Timestamp that)const{
    return this->microSecondsSinceEpoch_ < that.microSecondsSinceEpoch_;
}
bool Timestamp::operator>(const Timestamp that)const{
    return this->microSecondsSinceEpoch_ > that.microSecondsSinceEpoch_;
}