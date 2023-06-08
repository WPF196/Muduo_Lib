#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// channel的成员index_（初始化为-1）
const int kNew = -1;    // 一个channel还没有添加到poller之中
const int kAdded = 1;   // 一 个channel已经添加到poller之中
const int kDeleted = 2; // 一个channel已经在poller之中删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)  // vector<epoll_event>
{
    if (epollfd_ < 0)
        LOG_FATAL("epoll_create error:%d \n", errno);
}

EPollPoller::~EPollPoller() 
{
    ::close(epollfd_);
}

// 主要作用是调用epoll_wait
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // LOG_DEBUG更为合理，此处用INFO为了方便看到结果
    LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                            static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        LOG_INFO("%d events happend \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);   
        if(numEvents == events_.size())  // 说明所有的事件都发生了，需要扩容
        {   
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)  // timeout超时
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else                     // 发生了错误
    {           
        if(saveErrno != EINTR)      // 错误信息 != 外部中断
        {
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() err!");
        }
    }
    return now;
}

// channel update => EventLoop updateChannel => Poller updateChannel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n",
         __FUNCTION__, channel->fd(), channel->events(), index);
    
    // 新加入或者已删除
    if(index == kNew || index == kDeleted)
    {   
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else    // channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())  // 对任何事件都不感兴趣，删除即可
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else                       // 换了感兴趣的事件
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// channel remove => EventLoop removeChannel => Poller removeChannel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d \n", __FUNCTION__, fd);

    int index = channel->index();
    if(index == kAdded)         // 如果是已加过的，则在epoll中也需要删除
    {
        update(EPOLL_CTL_DEL, channel); 
    }
    channel->set_index(kNew);   // 设置从未注册过的状态
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);// EventLoop拿到了它的poller给他返回的所有发生事件的列表
    }
}

// epoll_ctl  add/mod 
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}