#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

/**
 * noncopyable被继承之后，派生类可以正常的构造和析构，
 * 但是派生类对象无法进行拷贝和赋值操作
*/
class noncopyable
{
public:
    // 删除 拷贝构造 和 赋值重载 
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
    
protected:
    // 默认实现 构造 和 析构
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif