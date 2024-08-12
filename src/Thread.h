#pragma once

#include "nocopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <atomic>
#include <string>
#include <unistd.h>

class Thread : nocopyable
{
public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc, const std::string &name = std::string());
  ~Thread();

  void start();
  void join();

  bool started() const { return started_; }
  pid_t tid() const { return tid_; }

  const std::string &name() const { return name_; }
  static int numCreated() { return numCreated_; }

private:
  void setDefaultName();

private:
  bool started_;
  bool joined_;

  std::shared_ptr<std::thread> thread_; // TODO: 分析muduo封装的posix Thread
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;
  static std::atomic_int numCreated_;
};