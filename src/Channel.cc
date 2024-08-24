#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel() {}

void Channel::handleEvent(Timestamp receiveTime)
{
  if (tied_)
  {
    // 确保channel对象存在，创建shared_ptr，如果tie_不存在则返回空的shared_ptr
    std::shared_ptr<void> guard = tie_.lock();
    if (guard)
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    // TODO
    handleEventWithGuard(receiveTime);
  }
}

// TODO
void Channel::tie(const std::shared_ptr<void> &obj)
{
  tie_ = obj;
  tied_ = true;
}

// 删除当前的channel，通过eventloop调用poller
void Channel::remove()
{
  loop_->removeChannel(this);
}

// 改变fd的events后，通过eventloop调用poller设置epoll_ctl
void Channel::update()
{
  loop_->updateChannel(this);
}
// 根据poller通知的事件，调用该channel的回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  LOG_INFO("channel handle events %d", revents_)

  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
  {
    LOG_INFO("handle HUP");
    if (closeCallback_)
    {
      LOG_INFO("closeCallback_ ----------");
      closeCallback_();
    }
  }

  if (revents_ & EPOLLERR)
  {
    LOG_INFO("handle ERR");
    if (errorCallback_)
    {
      LOG_INFO("errorCallback_ ----------");
      errorCallback_();
    }
  }

  if (revents_ & (EPOLLIN | EPOLLPRI))
  {
    LOG_INFO("handle IN");
    if (readCallback_)
      readCallback_(receiveTime);
  }

  if (revents_ & EPOLLOUT)
  {
    LOG_INFO("handle WRITE");
    LOG_INFO("writecallback_ address: %p", writeCallback_);
    if (writeCallback_)
    {
      LOG_INFO("writeCallback_ ----------");
      writeCallback_();
    }
  }
}
