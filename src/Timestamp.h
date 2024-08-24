#pragma once

#include "copyable.h"
#include <iostream>
#include <string>

class Timestamp : public copyable
{
public:
  Timestamp();
  explicit Timestamp(int64_t microSecondsSinceEpoch);
  static Timestamp now();
  std::string toString() const;
  static const int kMicroSecondsPerSecond = 1000 * 1000;
  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline Timestamp addTime(Timestamp timestamp, int sec)
{
  int64_t delta = static_cast<int64_t>(sec * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}