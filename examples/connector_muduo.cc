#include "muduo/net/TcpClient.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"

using namespace muduo;
using namespace muduo::net;

EventLoop *gloop;

int main()
{
  EventLoop loop;
  gloop = &loop;
  InetAddress addr(9999, "127.0.0.1");

  ConnectorPtr connector(new Connector(&loop, addr));
  connector->start();
  loop.loop();
  return 0;
}