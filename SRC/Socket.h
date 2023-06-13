#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();           // 关闭写操作

    void setTcpNoDelay(bool on);    // 不进行tcp缓冲，直接发送
    void setReuseAddr(bool on);     // 地址复用
    void setReusePort(bool on);     // 端口复用
    void setKeepAlive(bool on);     // 设置心跳包
private:
    const int sockfd_;
};