#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <memory>

/* _thread变量每一个线程有一份独立实体，各个线程的值互不干扰。
可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。*/
// 防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr; 

// 定义默认的Poller IO复用接口的超时时间（10s）
const int kPoolTimeMs = 10000;

// 创建wakeupfd，用来唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error: %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_)) 
    , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if(t_loopInThisThread)      // 当前线程已经有一个Loop了
    {
        LOG_FATAL("Another EventLoop %p exits in this thread %d \n",
                t_loopInThisThread, threadId_);
    }
    else                        // 如果第一次创建这个loop
    {
        t_loopInThisThread = this;  // 指向当前的loop
    }

    // 设置wakeupfd的事件类型已经发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读时间
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;        // 开启循环
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd（client_td和wakeup_fd）
        pollReturnTime_ = poller_->poll(kPoolTimeMs, &activeChannels_);
        for(Channel* channel : activeChannels_) // 遍历所有发生事件
        {
            // Poller能够监听那些channel发生了事件，然后上报给EventLoop，
            // 通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程 [mainLoop] -accept-> [新用户] -返回-> [fd] <-打包- [channel] -分发-> [subLoop]
         * mainLoop 事先注册一个回调cb（需要subloop来执行）
         * wakeup subloop后，执行之前mainloop注册的cb操作 
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 两种情况：1、loop在自己的线程中调用quit;  2、在非loop的线程中，调用loop的quit
/**
 *             mainLoop
 *                 |
 *     —————————————————————————
 *     ↓           ↓           ↓
 * subLoop1    subLoop2    subLoop3
*/
void EventLoop::quit()
{
    quit_ = true;
    if(!isInLoopThread())   // 在一个subloop中调用mainloop的quit
    {
        wakeup();           // 唤醒上方loop中的阻塞，让loop运行完 
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())    // 在当前loop线程中执行cb
    {
        cb();
    }
    else     // 在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);     // 直接构造（push_back是拷贝构造）
    }

    // 唤醒相应的需要执行上面回调操作的loop
    // 不在当前线程 || （当前loop正在执行回调，但是又有了新的回调）
    if(!isInLoopThread() || callingPendingFunctors_)    
    {
        wakeup();       // 唤醒loop所在线程
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;       // 8个字节
    ssize_t n = read(wakeupFd_, &one, sizeof one);  // n应该等于8
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

// 向wakeupfd写一个数据，wakeupChannel就会发生读事件，当前（阻塞的）loop线程空就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() write %lu bytes instead of 8\n", n)
    }
}

// EventLoop的方法 ==调用==> Poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors)
    {
        functor();      // 执行当前loop需要执行的回调操作 
    }

    callingPendingFunctors_ = false;
}