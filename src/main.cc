#include "Logger.h"

int main()
{
  struct timespec ts = {0, 0};
  ts.tv_sec = 0;
  ts.tv_nsec = 500000000;
  while (1)
  {
    LOG_INFO("%s", "你好");
    nanosleep(&ts, NULL);
  }

  return 0;
}