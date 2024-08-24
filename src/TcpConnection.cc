#include "TcpConnection.h"
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Logger.h"

#include <functional>
#include <sys/types.h>

void defaultConnectionCallback(const TcpConnectionPtr &conn)
{
  LOG_INFO("%s -> %s is %s", conn->localAddress().toIpPort().c_str(), conn->peerAddress().toIpPort().c_str(), conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr &,
                            Buffer *buf,
                            Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(loop),
      name_(name),
      stat_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

  LOG_INFO("TcpConnection::ctor[%s] at fd=%d", name.c_str(), sockfd)
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d", name_.c_str(), channel_->fd(), (int)stat_)
}

void TcpConnection::send(const std::string &buf)
{
  if (stat_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf.c_str(), buf.size());
    }
    else
    {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
    }
  }
}

void TcpConnection::shutdown()
{
  if (stat_ == kConnected)
  {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::connectEstablished()
{
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  if (stat_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::forceClose()
{
  if (stat_ == kConnected || stat_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop()
{
  if (stat_ == kConnected || stat_ == kDisconnecting)
  {
    handleClose();
  }
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0)
  {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = savedErrno;
    LOG_ERROR("TcpConnection::handleRead")
    handleError();
  }
}

void TcpConnection::handleWrite()
{
  if (channel_->isWriting())
  {
    int saveErrno = 0;
    ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)
      {
        channel_->disableWriting();
        if (writeCompleteCallback_)
        {
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (stat_ == kDisconnecting)
        {
          shutdownInLoop();
        }
      }
    }
  }
}

void TcpConnection::handleClose()
{
  LOG_INFO("TcpConnection::handleClose fd=%d state=%d", channel_->fd(), (int)stat_)
  setState(kDisconnected);
  channel_->disableAll();
  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);
  closeCallback_(connPtr);
}

void TcpConnection::handleError()
{
  int optval;
  socklen_t optlen = sizeof(optval);
  int err = 0;
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
  {
    err = errno;
  }
  else
  {
    err = optval;
  }
  LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d", name_.c_str(), err)
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;

  if (stat_ == kDisconnected)
  {
    LOG_ERROR("disconnected, can't wrting")
    return;
  }

  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else
    {
      nwrote = 0;
      if (errno != EAGAIN)
      {
        LOG_ERROR("TcpConnection::sendInLoop")
        if (errno == EPIPE || errno == ECONNRESET)
        {
          faultError = true;
        }
      }
    }
  }

  if (!faultError && remaining > 0)
  {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
    {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append((char *)data + nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdownInLoop()
{
  // 如果socket上有数据没写完则暂时不关闭
  if (!channel_->isWriting())
  {
    socket_->shutdownWrite();
  }
}
