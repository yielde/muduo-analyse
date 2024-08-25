#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include <sys/socket.h>
#include <strings.h>
#include <sys/timerfd.h>
#include <time.h>

static int createNonblocking()
{
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (sockfd < 0)
  {
    LOG_FATAL("Connector create fd failed: %d", errno);
    abort();
  }
  return sockfd;
}

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
}

Connector::~Connector()
{
  LOG_ERROR("Connector died ~~~~");
}

void Connector::start()
{
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::restart()
{
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_ERROR("do not connect");
  }
}

void Connector::stop()
{
  connect_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::startInLoop()
{
  LOG_INFO("startinloop state: %d", state_);
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_ERROR("do not connect");
  }
}

void Connector::stopInLoop()
{
  if (state_ == kConnecting)
  {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
}

void Connector::connect()
{
  int sockfd = createNonblocking();
  int ret = ::connect(sockfd, (sockaddr *)serverAddr_.getSockAddr(), static_cast<socklen_t>(sizeof(sockaddr_in)));
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
  case 0:
  case EINPROGRESS:
  case EINTR:
  case EISCONN:
    LOG_INFO("connecting --- %d", savedErrno);
    connecting(sockfd);
    break;

  case EAGAIN:
  case EADDRINUSE:
  case EADDRNOTAVAIL:
  case ECONNREFUSED:
  case ENETUNREACH:
    LOG_INFO("EAGAIN retry ---");
    retry(sockfd);
    break;

  case EACCES:
  case EPERM:
  case EAFNOSUPPORT:
  case EALREADY:
  case EBADF:
  case EFAULT:
  case ENOTSOCK:
    LOG_ERROR("Connector error: %d", savedErrno);
    if (::close(sockfd) < 0)
    {
      LOG_ERROR("close socket %d error", sockfd);
    }
    break;

  default:
    LOG_ERROR("Connector with unexception error: %d", savedErrno);
    if (::close(sockfd) < 0)
    {
      LOG_ERROR("close socket %d error", sockfd);
    }
    break;
  }
}

void Connector::connecting(int sockfd)
{
  setState(kConnecting);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(std::bind(&Connector::handleWriter, this));
  channel_->setErrorCallback(std::bind(&Connector::handleError, this));
  // channel_->tie(shared_from_this());
  channel_->enableWriting();
}

void Connector::handleWriter()
{
  LOG_INFO("Connector::handleWrite, state: %d", state_);
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int optval;
    socklen_t optlen = sizeof(optval);
    int err;
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
      err = errno;
    }
    else
    {
      err = optval;
    }
    if (err)
    {
      LOG_ERROR("Connector::handleWriter - SO_ERROR = %d", err);
      retry(sockfd);
    }
    else if ([=](int fd)
             {
      struct sockaddr_in localaddr;
      bzero(&localaddr, sizeof(localaddr));
      socklen_t locallen = sizeof(localaddr);
      ::getsockname(fd, (sockaddr *)&localaddr, &locallen);

      struct sockaddr_in peeraddr;
      bzero(&peeraddr, sizeof(peeraddr));
      socklen_t peerlen = sizeof(peeraddr);
      ::getpeername(fd, (sockaddr *)&peeraddr, &peerlen);

      return localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr && localaddr.sin_port == peeraddr.sin_port; }(sockfd))
    {
      LOG_ERROR("Connector::handleWrite - self connect");
      retry(sockfd);
    }
    else
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);
      }
      else
      {
        ::close(sockfd); // need error handle
      }
    }
  }
  else
  {
    if (state_ == kDisconnected)
    {
      LOG_INFO("Connector::writeHandler disconnected");
    }
    else
    {
      LOG_ERROR("Connector::writeHandler unexception error");
      exit(-1);
    }
  }
}

void Connector::handleError()
{
  LOG_ERROR("Connector::handleError state=%d %d", state_, errno);
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err;
    socklen_t errlen = sizeof(err);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    LOG_ERROR("Connector::handleError errno: %d", err);
    retry(sockfd);
  }
}

void Connector::retry(int sockfd)
{
  if (::close(sockfd) < 0)
  {
    LOG_ERROR("close socket %d error", errno);
  }
  setState(kDisconnected);
  if (connect_)
  {
    LOG_INFO("Connector::retry - Retry connecting to %s in %d", serverAddr_.toIpPort().c_str(), retryDelayMs_);
    // loop_->runInLoop(std::bind(&Connector::startInLoop, shared_from_this()));
    // loop_->queueInLoop(std::bind(&Connector::startInLoop, shared_from_this()));
    loop_->runAfter(5, std::bind(&Connector::startInLoop, shared_from_this()));
  }
  else
  {
    LOG_INFO("do not connect");
  }
}

int Connector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
  return sockfd;
}

void Connector::resetChannel()
{
  channel_.reset();
}
