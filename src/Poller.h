#pragma once

#include "nocopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// 基类，对外统一接口负责将channel的fd和事件注册到io multiplex，将监测到的事件返回给channel
// epoll、poll、select可继承该poller
class Poller : nocopyable
{
public:
  using ChannelList = std::vector<Channel *>;

  Poller(EventLoop *loop);
  virtual ~Poller() = default;

  virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
  virtual void updateChannel(Channel *channel) = 0;
  virtual void removeChannel(Channel *channel) = 0;
  bool hasChannel(Channel *channel) const;

  // 启动Poller，具体使用的是Epoll接口还是Poll接口
  // 实现在DefaultPoller.cc中，因为要拿到派生类的实例，基类最后不要include派生类
  static Poller *newDefaultPoller(EventLoop *loop);

protected:
  using ChannelMap = std::unordered_map<int, Channel *>; // 文件fd和channel映射表
  ChannelMap channels_;                                  // 添加到poller的channel

private:
  EventLoop *ownerLoop_; // Poller所属的Eventloop
};