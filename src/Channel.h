#pragma once

#include "nocopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

class Channel : nocopyable
{
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;
  Channel(EventLoop *loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);

  // 将函数对象cb转换为右值引用，省去了拷贝cb的开销
  void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  void tie(const std::shared_ptr<void> &); // 管理channel的生命周期，防止执行回调操作的过程中，channel被delete掉
  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }

  // 设置events位，muduo同时支持epoll与poll
  // epoll与poll相同功能的位相同，例如EPOLLIN == POLLIN
  void enableReading()
  {
    events_ |= kReadEvent;
    update();
  }

  void disableReading()
  {
    events_ &= ~kReadEvent;
    update();
  }

  void enableWriting()
  {
    events_ |= kWriteEvent;
    update();
  }

  void disableWriting()
  {
    events_ &= ~kWriteEvent;
    update();
  }

  void disableAll()
  {
    events_ = kNoneEvent;
    update();
  }

  // 返回fd当前事件状态
  bool isNoneEvent() { return events_ == kNoneEvent; }
  bool isWriting() { return events_ & kWriteEvent; }
  bool isReading() { return events_ & kReadEvent; }

  int index() { return index_; }
  void set_index(int idx) { index_ = idx; } // 更新channel在poller中的状态（new added deleted）

  EventLoop *ownerLoop() { return loop_; } // one loop per thread
  void remove();

private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

private:
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *loop_;
  const int fd_; // channel对象的fd
  int events_;   // 注册的事件
  int revents_;  // poller 返回的事件
  int index_;    // 当前channel在poller中的状态（new Added Deleted）
  bool logHup_;

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;
  bool addedToLoop_;

  // 调用具体事件的回调
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};