#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }
private:
    void setDefaultName();  //给线程设置默认名称

    bool started_;
    bool joined_;
    // 需要用智能指针，从而控制线程（常规声明后则直接启动线程）
    std::shared_ptr<std::thread> thread_;   
    pid_t tid_;
    ThreadFunc func_;       // 线程函数
    std::string name_;      // 线程名
    static std::atomic_int numCreated_;     // 线程计数
};