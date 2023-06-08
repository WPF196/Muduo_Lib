#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <string>

// 时间类
class Timestamp
{
public:
    Timestamp();
    // 防止隐式类型转换
    explicit Timestamp(int64_t microSecondsSinceEpoch); 
    static Timestamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};


#endif