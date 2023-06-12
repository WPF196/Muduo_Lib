#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop; 

/**
 * EventLoop < 1---1 > Poller < 1---n > Channel （此三者构成Reactor模型上的事件分发器）
 * Channel 相当于一个文件描述符的保姆，封装了sockfd和其感兴趣的event，
 * 如EPOLLIN、EPOLLOUT事件。还绑定了poller返回的具体事件
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;  
    using ReadEventCallback = std::function<void(Timestamp)>;   

    Channel(EventLoop* loop, int fd);   // 参数：channel所在的loop和cannel的fd
    ~Channel();

    // fd得到poller通知以后，处理事件（回调函数）
    void handleEvent(Timestamp receiveTime);

    // 向Channel对象注册各类事件的处理函数（调用转换的右值，节约内存资源）
    void setReadCallback(ReadEventCallback cb){ readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb){ writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb){ closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb){ errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    // poller监听到某个文件描述符发生事件，将这个文件描述符实际发生的事件封装进这个Channel
    void set_revents(int revt) { revents_ = revt; } 

    // 将Channel中的文件描述符及其感兴趣事件注册到Poller上或从Poller移除
    void enableReading() { events_ |= kReadEvent; update(); }   // 添加kReadEvent
    void disableReading() { events_ &= ~kReadEvent; update(); } // 去除kReadEvent
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;    // 无事件感兴趣
    static const int kReadEvent;    // 读事件感兴趣
    static const int kWriteEvent;   // 写事件感兴趣

    EventLoop *loop_;               // 事件循环
    const int fd_;                  // Poller监听的对象
    int events_;                    // fd感兴趣的事件类型集合
    int revents_;                   // 事件监听器实际监听到该fd发生的事件类型集合
    int index_;                     // 事件状态

    std::weak_ptr<void> tie_;
    bool tied_;

    /* 因为channel通道里面能够获知fd最终发生的具体事件revents，
       所以它负责调用具体时间的回调操作 */
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};