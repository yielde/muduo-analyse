#pragma once

#include "nocopyable.h"
#include "InetAddress.h"
#include <memory>
#include <functional>
#include <atomic>
#include <unistd.h>

class Channel;
class EventLoop;

class Connector : nocopyable,
                  public std::enable_shared_from_this<Connector>
{
public:
  using NewConnectionCallback = std::function<void(int sockfd)>;

  Connector(EventLoop *loop, const InetAddress &serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback &cb)
  {
    newConnectionCallback_ = cb;
  }

  void start();
  void restart();
  void stop();

  const InetAddress &serverAddress() const { return serverAddr_; }

private:
  enum States
  {
    kDisconnected,
    kConnecting,
    kConnected
  };
  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWriter();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

private:
  static const int kMaxRetryDelayMs = 30 * 1000;
  static const int kInitRetryDelayMs = 500;
  EventLoop *loop_;
  InetAddress serverAddr_;
  std::atomic_bool connect_;
  States state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
};