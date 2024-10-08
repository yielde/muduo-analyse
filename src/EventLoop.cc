#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "TimerQueue.h"
#include <sys/eventfd.h>
#include <memory>

__thread EventLoop *t_loopInThisThread = nullptr; // 一个线程只能创建一个eventloop

const int kPollTimeMs = 10000; // Poller超时时间，即epoll_ctl

int createEventfd()
{
  int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evfd < 0)
  {
    LOG_FATAL("create eventfd error: %d", errno)
    abort();
  }
  return evfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),         // 获取当前EventLoop的tid
      poller_(Poller::newDefaultPoller(this)), // 将该eventloop与poller绑定
      wakeupFd_(createEventfd()),              // 通过eventfd实现唤醒subreactor处理channel，还可以socketpair来做
      wakeupChannel_(new Channel(this, wakeupFd_)),
      timerQueue_(new TimerQueue(this))
{
  LOG_DEBUG("EventLoop created %p in thread %d", this, threadId_)
  if (t_loopInThisThread)
  {
    LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_)
    exit(-1);
  }
  else
  {
    t_loopInThisThread = this;
  }

  // TODO
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));

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
  looping_ = true;
  quit_ = false;

  LOG_INFO("EventLoop %p start looping", this)

  while (!quit_)
  {
    activeChannels_.clear();
    // 交给poller将epoll_wait的channel返回到activeChannels_
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    for (Channel *channel : activeChannels_)
    {
      // 在poller中已经将channel的_revents修改，直接调用channel处理事件
      channel->handleEvent(pollReturnTime());
    }

    doPendingFunctors();
  }

  LOG_INFO("EventLoop %p stop looping", this)
  looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(Functor cb)
{
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb);
  }

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one))
  {
    LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8", n)
  }
}

void EventLoop::updateChannel(Channel *channel)
{
  // channel通过EventLoop调用Poller将channel的fd和events加入到epoll，如果没有events，将该channel从epoll删除
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
  // channel通过EventLoop调用Poller将channel的fd从epoll删除
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
  return poller_->hasChannel(channel);
}

TimerId EventLoop::runAt(Timestamp time, TimerCallbck cb)
{
  return timerQueue_->addTimer(std::move(cb), time, 0);
}

TimerId EventLoop::runAfter(int delay, TimerCallbck cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one))
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n)
  }
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors; // 函数对象数组
  callingPendingFunctors_ = true;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor &functor : functors)
  {
    functor(); // 当前loop需要执行的callback
  }

  callingPendingFunctors_ = false;
}
