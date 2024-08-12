#pragma once

#include "nocopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread : nocopyable
{
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;
  // 避免对象拷贝，但仍然可以使用临时对象，临时对象的声明周期延绑定到cb、name
  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());

  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadFunc();
  EventLoop *loop_;

  bool exiting_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};