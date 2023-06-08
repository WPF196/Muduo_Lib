#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * epoll的使用  
 * epoll_create ----------------> 构造、析构
 * epoll_ctl (add/mod/del) -----> updateChannel、removeChannel
 * epoll_wait ------------------> poll函数
 */ 
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);       // epoll_create
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override; // epoll_wait
    void updateChannel(Channel *channel) override;  // 调用update 
    void removeChannel(Channel *channel) override;  // epoll_ctl  del
private:
    static const int kInitEventListSize = 16;       // EventList的初始长度

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道（在channel里设置感兴趣的事件） 
    void update(int operation, Channel *channel);   // epoll_ctl  add + mod 

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};