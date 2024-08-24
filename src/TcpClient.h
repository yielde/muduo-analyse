#pragma once

#include "TcpConnection.h"
#include <mutex>
class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient
{
public:
  TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &nameArg);
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const;
  EventLoop *getLoop() const { return loop_; }
  bool retry() const { return retry_; }
  bool enableRetry() { retry_ = true; }

  const std::string &name() const { return name_; }

  void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
  void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
  void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

private:
  void newConnection(int sockfd);
  void removeConnection(const TcpConnectionPtr &conn);

private:
  std::mutex mutex_;
  EventLoop *loop_;
  ConnectorPtr connector_;
  const std::string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  std::atomic_bool retry_;
  std::atomic_bool connect_;
  int nextConnId_;
  TcpConnectionPtr connection_;
};
