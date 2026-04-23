#pragma once
#include <iostream>
#include <ctime>
#include <sys/time.h>
namespace muduo{
    namespace base{
        class Timestamp{
            private:
             int64_t microSecondsSinceEpoch_;
            public:
             static const int kMicroSecondsPerSecond = 1000000;
        }
    }
}