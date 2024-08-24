#include "TcpClient.h"
#include "Connector.h"
#include "Logger.h"
#include "TcpConnection.h"
#include "EventLoop.h"

#include <sys/socket.h>

namespace detail
{
  void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
  {
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }
};

TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
  connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
  LOG_INFO("TcpClient::TcpClient[%s] - connector %p", name_, connector_.get());
}

TcpClient::~TcpClient()
{
  LOG_INFO("TcpClient::~TcpClient[%s] - connector %p", name_, connector_.get());
  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn)
  {
    CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
    if (unique)
    {
      conn->forceClose();
    }
  }
  else
  {
    connector_->stop();
  }
}

void TcpClient::connect()
{
  LOG_INFO("TcpClient::connect[%s] - connecting to %s", name_, connector_->serverAddress().toIpPort().c_str());
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
{
  connect_ = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}
void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
  struct sockaddr_in peer;
  socklen_t peerAddrLen = sizeof(peer);
  getpeername(sockfd, (sockaddr *)&peer, &peerAddrLen);
  InetAddress peerAddr(peer);
  char buf[32];
  snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  struct sockaddr_in local;
  socklen_t localAddrLen = sizeof(local);
  getsockname(sockfd, (sockaddr *)&local, &localAddrLen);
  InetAddress localAddr(local);

  TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_.reset();
  }
  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    LOG_INFO("TcpClient::connect[%s] - Reconnecting to %s", name_, connector_->serverAddress().toIpPort().c_str());
    connector_->restart();
  }
}