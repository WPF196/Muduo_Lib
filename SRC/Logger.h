#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#include "noncopyable.h"

// LOG_XXXX("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::getInstance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0);

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::getInstance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0);

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::getInstance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(1); \
    }while(0);

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::getInstance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0);
#else 
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志级别
enum LogLevel
{
    INFO,       // 日志正常输出
    ERROR,      // 错误信息（不影响软件继续执行）
    FATAL,      // core信息（软件崩溃）
    DEBUG,      // 调试信息
};

// 输出一个日志的类
class Logger : noncopyable
{
public:
    static Logger& getInstance();   // 获取日志唯一的实例化对象
    void setLogLevel(int);          // 设置日志等级
    void log(std::string);          // 写日志

private:
    Logger(){}                  // 单例，私有构造
    static Logger* instance_;   // 唯一实例对象

    int logLevel_;              // 日志等级
};

#endif