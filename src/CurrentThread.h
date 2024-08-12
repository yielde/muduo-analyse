#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
  extern __thread int t_cachedTid;
  void cacheTid();

  inline int tid()
  {
    // gcc优化，告诉编译器t_cachedTid == 0，对cpu pipeline友好（pipeline先推入return t_cachedTid指令，不用jmp）
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }
}