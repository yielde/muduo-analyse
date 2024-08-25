#include <yieldemuduo/TcpServer.h>
#include <yieldemuduo/Logger.h>
#include <yieldemuduo/Timer.h>

#include <string>
#include <functional>

class EchoServer
{
public:
  EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name) : server_(loop, addr, name), loop_(loop)
  {
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    server_.setThreadNum(4);
  }

  void start()
  {
    server_.start();
  }

private:
  void onConnection(const TcpConnectionPtr &conn)
  {
    if (conn->connected())
    {
      LOG_INFO("Connection UP: %s", conn->peerAddress().toIpPort().c_str());
    }
    else
    {
      LOG_INFO("Connection DOWN: %s", conn->peerAddress().toIpPort().c_str());
    }
  }

  void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
  {
    std::string msg = buf->retrieveAllAsString();
    sleep(3);
    conn->send(time.toString());
    LOG_INFO("my message %s", msg.c_str());
    // conn->shutdown();
  }

private:
  EventLoop *loop_;
  TcpServer server_;
};

int main()
{
  EventLoop loop;
  InetAddress addr(9999, "0.0.0.0");
  EchoServer server(&loop, addr, "EchoServer-01");
  server.start();
  loop.loop();
  return 0;
}