#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
using TimerCallbck = std::function<void()>;

void defaultConnectionCallback(const TcpConnectionPtr &conn);
void defaultMessageCallback(const TcpConnectionPtr &conn,
                            Buffer *buffer,
                            Timestamp receiveTime);