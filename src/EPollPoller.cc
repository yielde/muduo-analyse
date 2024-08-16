#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <unistd.h>
#include <errno.h>
#include <strings.h>

// channel在poller中的状态，
const int kNew = -1;    // channel未添加到poller中
const int kAdded = 1;   // channel已经添加到poller中
const int kDeleted = 2; // channel已经从poller中删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
  if (epollfd_ < 0)
  {
    LOG_FATAL("epoll_create1 error: %d", errno)
    abort();
  }
}

EPollPoller::~EPollPoller()
{
  ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
  LOG_DEBUG("func=%s -> fd total count: %lu\n", __FUNCTION__, channels_.size())
  int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
  int saveErrno = errno;
  Timestamp now(Timestamp::now());

  if (numEvents > 0)
  {
    LOG_INFO("%d events happend", numEvents)
    fillActiveChannels(numEvents, activeChannels);
    if (numEvents == events_.size())
    {
      events_.resize(events_.size() * 2);
    }
  }
  else if (numEvents == 0)
  {
    LOG_DEBUG("nothing happend!")
  }
  else
  {
    if (saveErrno != EINTR)
    {
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll() error!")
    }
  }
  return now;
}

void EPollPoller::updateChannel(Channel *channel)
{
  const int index = channel->index();
  LOG_INFO("func=%s -> fd=%d -> events=%d -> index=%d", __FUNCTION__, channel->fd(), channel->events(), index)
  // 是新的channel或者是之前删除过的channel
  if (index == kNew || index == kDeleted)
  {
    if (index == kNew)
    {
      int fd = channel->fd();
      channels_[fd] = channel;
    }

    update(EPOLL_CTL_ADD, channel);
    channel->set_index(kAdded);
  }
  else
  {
    // channel已经在poller上注册过了
    int fd = channel->fd();
    // 没有要关注的事件，删除掉channel
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      // 修改channel关注的事件
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::removeChannel(Channel *channel)
{
  int fd = channel->fd();
  channels_.erase(fd);
  LOG_INFO("func=%s -> fd=%d", __FUNCTION__, fd)
  int index = channel->index();
  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
  for (int i = 0; i < numEvents; ++i)
  {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    // 给channel设置fd发生的events
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::update(int operation, Channel *channel)
{
  epoll_event event;
  bzero(&event, sizeof(event));
  int fd = channel->fd();
  event.events = channel->events();
  event.data.fd = fd;
  event.data.ptr = channel;

  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_ERROR("epoll_ctl del error: %d", errno)
    }
    else
    {
      LOG_FATAL("epoll_ctl add/mod error: %d", errno)
      abort();
    }
  }
}
