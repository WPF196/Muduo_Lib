#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {   
        // __builtin_expect(EXP, N)：优化分支，意思是EXP==N的概率很大
        if(__builtin_expect(t_cachedTid == 0, 0))   // 为获取tid 
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}