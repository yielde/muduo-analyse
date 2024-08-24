#include "yieldemuduo/TcpClient.h"
#include "yieldemuduo/EventLoop.h"
#include "yieldemuduo/Connector.h"

EventLoop *gloop;

void connectCallback(int sockfd)
{
  gloop->quit();
}

int main()
{
  EventLoop loop;

  gloop = &loop;
  InetAddress addr(9999, "127.0.0.1");

  ConnectorPtr connector(new Connector(&loop, addr));
  connector->setNewConnectionCallback(connectCallback);
  connector->start();
  loop.loop();
  return 0;
}