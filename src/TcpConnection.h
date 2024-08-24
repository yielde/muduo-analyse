#pragma once

#include "nocopyable.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "Buffer.h"
#include "Callbacks.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

// 返回指向TcpConnection的shared_ptr
// 如果使用shared_ptr<TcpConnection> ptr1(this),shared_ptr<TcpConnection> ptr2(this), ptr1 = ptr2 会多次调用shared_ptr构造函数
// ptr1和ptr2的 _Ref_count_base 都为1，element_type都是指向该对象内存，会重复析构该对象
// 所以使用enable_shared_from_this的shared_from_this返回当前对象的shared_ptr，引用计数为2

// muduo要确保TcpConnection的生命周期长于
class TcpConnection : nocopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return localAddr_; }
  const InetAddress &peerAddress() const { return peerAddr_; }

  bool connected() const { return stat_ == kConnected; }

  void send(const std::string &buf);
  void shutdown();

  // TODO: 为什么没使用std::move
  void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = cb; }
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

  void connectEstablished();
  void connectDestroyed();
  void forceClose();
  void forceCloseInLoop();

private:
  enum StateE
  {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting
  };

  void setState(StateE state) { stat_ = state; }

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const void *message, size_t len);
  void shutdownInLoop();

private:
  EventLoop *loop_;
  const std::string name_;
  std::atomic_int stat_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress localAddr_;
  const InetAddress peerAddr_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  size_t highWaterMark_;

  Buffer inputBuffer_;
  Buffer outputBuffer_;
};