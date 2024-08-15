#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0), next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
  started_ = true;
  for (int i = 0; i < numThreads_; ++i)
  {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
    EventLoopThread *t = new EventLoopThread(cb, buf);
    // unique_ptr管理new 出来的EventLoopThread对象，成员变量vector自动析构，内部的EventLoopThread对象析构
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }

  // 没有开启多线程，只有一个线程EventLoop
  if (numThreads_ == 0 && cb)
  {
    cb(baseLoop_);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
  EventLoop *loop = baseLoop_;

  if (!loops_.empty())
  {
    loop = loops_[next_];
    ++next_;
    if (next_ >= loops_.size())
    {
      next_ = 0;
    }
  }

  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
  if (!loops_.empty())
  {
    return loops_;
  }
  else
  {
    return std::vector<EventLoop *>(1, baseLoop_);
  }
}
