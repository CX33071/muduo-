#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <ctime>
#include <sys/time.h>

namespace mulib{
    namespace base{
        class Timestamp{
        public:
            static const int kMicroSecondsPerSecond = 1000000;
            Timestamp() : microSecondsSinceEpoch_(0) {}
            explicit Timestamp(int64_t microSecondsSinceEpochArg)
                : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}
            static Timestamp now();
            void swap(Timestamp &that);
            bool valid() const;
            int64_t microSecondsSinceEpoch() const;
            time_t secondsSinceEpoch() const;
            std::string toFormattedString(bool showMicroseconds = true) const;
            static Timestamp addTime(Timestamp timestamp, double seconds);
            inline double timeDifference(Timestamp high, Timestamp low);
            
            bool operator==(const Timestamp that) const;
            bool operator<(const Timestamp that) const;
            bool operator>(const Timestamp that) const;
            ~Timestamp() = default;

        private:
            int64_t microSecondsSinceEpoch_;
        };
    }
}

#endif

//秒到微秒的转换
//比较两个Timestamp类对象
