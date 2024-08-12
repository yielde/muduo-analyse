#pragma once

#include <functional>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>

#include "nocopyable.h"
#include "Timestamp.h"

class Channel;
class Poller;

// Reactor
class EventLoop : nocopyable
{
public:
  using Functor = std::function<void()>;
  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  Timestamp pollReturnTime() const { return pollReturnTime_; }

  void runInLoop(Functor cb);
  void queueInLoop(Functor cb);

  void wakeup();

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
  void handleRead();
  void doPendingFunctors();

private:
  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_;
  std::atomic_bool quit_;

  const pid_t threadId_; // 创建当前EventLoop的thread

  std::mutex mutex_; // TODO:分析muduo Mutex类实现

  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;

  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;
  ChannelList activeChannels_; // poller返回的有就绪事件的channel

  std::atomic_bool callingPendingFunctors_;
  std::vector<Functor> pendingFunctors_;
};