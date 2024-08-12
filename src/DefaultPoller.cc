#include "Poller.h"
#include "EpollPoller.h"
#include <stdlib.h>
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
  if (::getenv("MUDUO_USE_POLL"))
  {
    return nullptr; // 暂不支持poll
  }
  else
  {
    return new EPollPoller(loop);
  }
}