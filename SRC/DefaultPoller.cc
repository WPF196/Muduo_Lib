#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

// 创建一个Poller实例，参数：代表当前poller所属的loop
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;     // 生成poll实例
    }
    else{
        return new EPollPoller(loop);     // 生成epoll实例
    }
}
