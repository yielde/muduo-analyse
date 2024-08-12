#pragma once

#include "Poller.h"
#include <sys/epoll.h>

class EPollPoller : public Poller
{
public:
  EPollPoller(EventLoop *loop);
  ~EPollPoller() override;
  // epoll_wait
  Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
  void updateChannel(Channel *channel) override; // channel调用eventloop告知poller执行操作
  void removeChannel(Channel *channel) override; // channel调用eventloop告知poller执行操作

private:
  static const int kInitEventListSize = 16; // 注册到poller的channel个数初始值（TODO）

  //  eventloop拿到当前有事件发生的channel
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

  // poller设置channel，即epoll_ctl设置fd
  void update(int operation, Channel *channel);

  using EventList = std::vector<epoll_event>;

  int epollfd_;      // 该poller的fd
  EventList events_; // epoll_wait返回的events
};