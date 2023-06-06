#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

const int kNew = -1;    // 一个channel还没有添加到poller之中
const int kAdded = 1;   // 一个channel已经添加到poller之中
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