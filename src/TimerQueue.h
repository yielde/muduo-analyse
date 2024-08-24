#pragma once
#include "Callbacks.h"
#include "Timestamp.h"
#include "Channel.h"
#include <set>
#include <vector>

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : nocopyable
{
public:
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  TimerId addTimer(TimerCallbck cb, Timestamp when, int interval);

private:
  using Entry = std::pair<Timestamp, Timer *>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer *, int64_t>; // 用于cancel
  using ActiveTimerSet = std::set<ActiveTimer>;    // 用于cancel

  void addTimerInLoop(Timer *timer);
  void reset(const std::vector<Entry> &expired, Timestamp now);
  bool insert(Timer *timer);
  void handleRead();
  std::vector<Entry> getExpired(Timestamp now);

private:
  EventLoop *loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  TimerList timers_;
  ActiveTimerSet activeTimers_;
  std::atomic_bool callingExpiredTimers_;
};