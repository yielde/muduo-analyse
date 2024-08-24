#include "Timer.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <strings.h>
#include <algorithm>

int createTimerfd()
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
  {
    LOG_ERROR("Failed in timerfd_create");
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)
    microseconds = 100;

  struct timespec ts;
  ts.tv_sec = microseconds / Timestamp::kMicroSecondsPerSecond;
  ts.tv_nsec = (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000;

  return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t count;
  ssize_t n = ::read(timerfd, &count, sizeof(count));
  LOG_INFO("TimerQueue::handleread, timer called %d times at %s", count, now.toString().c_str());
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
  struct itimerspec newValue;
  struct itimerspec oldValue;
  bzero(&newValue, sizeof(newValue));
  bzero(&oldValue, sizeof(oldValue));
  newValue.it_value = howMuchTimeFromNow(expiration);
  ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
}

TimerQueue::TimerQueue(EventLoop *loop) : loop_(loop), timerfd_(createTimerfd()), timerfdChannel_(loop, timerfd_)
{
  timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
  timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
  timerfdChannel_.disableAll();
  timerfdChannel_.remove();
  ::close(timerfd_);
}

TimerId TimerQueue::addTimer(TimerCallbck cb, Timestamp when, int interval)
{
  Timer *timer = new Timer(std::move(cb), when, interval);
  loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
  bool erliestChanged = insert(timer);
  if (erliestChanged)
  {
    resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
  Timestamp nextExpire;
  for (const Entry &it : expired)
  {
    delete it.second;
  }
  if (!timers_.empty())
  {
    nextExpire = timers_.begin()->second->expiration();
  }
  if (nextExpire.microSecondsSinceEpoch() > 0)
  {
    resetTimerfd(timerfd_, nextExpire);
  }
}

bool TimerQueue::insert(Timer *timer)
{
  bool erliestChanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first)
  {
    erliestChanged = true;
  }

  timers_.insert(Entry(when, timer));
  return erliestChanged;
}

void TimerQueue::handleRead()
{
  Timestamp now(Timestamp::now());
  readTimerfd(timerfd_, now);
  std::vector<Entry> expired = getExpired(now);
  callingExpiredTimers_ = true;
  for (const Entry &it : expired)
  {
    it.second->run();
  }
  callingExpiredTimers_ = false;
  reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
  TimerList::iterator end = timers_.lower_bound(sentry);
  std::copy(timers_.begin(), end, std::back_inserter(expired));
  timers_.erase(timers_.begin(), end);
  return expired;
}
