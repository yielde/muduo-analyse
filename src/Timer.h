#pragma once

#include "Callbacks.h"
#include "Timestamp.h"
#include "nocopyable.h"
#include <atomic>

class Timer : nocopyable
{
public:
  Timer(TimerCallbck cb, Timestamp when, int interval)
      : callback_(std::move(cb)),
        expiration_(when),
        interval_(interval),
        repeat_(interval_ > 0),
        sequence_(++s_numCreated_)
  {
  }

  void run() const
  {
    callback_();
  }

  Timestamp expiration() const { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }
  void restart(Timestamp now);
  static int64_t numCreated() { return s_numCreated_; }

private:
  const TimerCallbck callback_;
  Timestamp expiration_;
  const int interval_;
  const bool repeat_;
  const int64_t sequence_;

  static std::atomic<int64_t> s_numCreated_;
};