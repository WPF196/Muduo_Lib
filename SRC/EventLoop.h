#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;


// 事件循环类 （两大模块：Channel  Poller（epoll的抽象））
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();    // 开启事件循环
    void quit();    // 退出事件循环

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    
    void runInLoop(Functor cb);     // 在当前loop中执行
    void queueInLoop(Functor cb);   // 把cb放入队列中，唤醒loop所在的线程，执行cb
    
    void wakeup();                  // 唤醒loop所在的线程

    // EventLoop的方法 ==调用==》 Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断EventLoop对象是否在自己的线程里
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private: 
    void handleRead();          // 唤醒
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;
    
    /* 原子变量，它能够确保对共享变量的操作在执行时不会
    被其他线程的操作干扰，从而避免竞态条件和死锁 */
    std::atomic_bool looping_;  
    std::atomic_bool quit_;         // 标志退出loop循环
    
    const pid_t threadId_;          // 记录当前loop所在线程的id
    
    Timestamp pollReturnTime_;      // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;
    
    int wakeupFd_;  // 当mainLoop获取一个新用户的channel，通过轮询，选择一个subLoop并唤醒之
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_;   // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;      // 存储loop需要执行的所有的回调操作
    std::mutex mutex_;                          // 保护上面vector容器的线程安全
};


#endif