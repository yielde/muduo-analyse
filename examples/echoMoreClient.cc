#include "yieldemuduo/TcpClient.h"
#include "yieldemuduo/EventLoop.h"
#include "yieldemuduo/Logger.h"
#include <iostream>

void onConnection(const TcpConnectionPtr &conn)
{
  if (conn->connected())
  {
    std::cout << "Connect server: " << conn->peerAddress().toIpPort() << " : " << "Connect client: " << conn->localAddress().toIpPort() << std::endl;
    conn->send("hello i am yielde");
  }
  else
  {
    std::cout << "NONONO can't connecting " << std::endl;
  }
}

void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
{
  // std::string message = buf->retrieveAllAsString();
  // std::string sendbuf = "i received message at time: " + time.toString();
  // conn->send(sendbuf);
  std::string message = buf->retrieveAllAsString();
  LOG_INFO(" receive message %s", message.c_str());
  conn->send(time.toString());
}

int main()
{
  EventLoop loop;
  InetAddress ServerAddr(9999, "10.211.55.3");
  std::string ClientName("yieldeClient");
  TcpClient client(&loop, ServerAddr, ClientName);
  client.connect();
  client.setConnectionCallback(std::bind(onConnection, std::placeholders::_1));
  client.setMessageCallback(std::bind(onMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  loop.loop();
  client.disconnect();
  return 0;
}