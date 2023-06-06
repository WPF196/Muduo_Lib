#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

Logger& Logger::getInstance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 格式: [级别信息] time : msg
void Logger::log(std::string msg)
{   
    // 级别
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    // 时间和msg 
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}