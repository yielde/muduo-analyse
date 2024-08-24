#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>
std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
  setDefaultName();
}

Thread::~Thread()
{
  if (started_ && !joined_)
  {
    thread_->detach();
  }
}

void Thread::start()
{
  started_ = true;
  // 使用c++11的semphore
  // muduo使用pthread_condition_t实现了一个countdownlatch传入线程，
  // 在主线程设置倒计时器为1，子线程初始化完毕后会countdown，count为0后通知主线程（pthread_cond_broadcast）
  sem_t sem;
  sem_init(&sem, false, 0);

  // 主线程和新起的线程同步，当前主线程会阻塞在sem_wait，如果新线程准备完毕，发送了sem_post，则新线程资源初始化完毕，可以正常使用
  thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                         { tid_ = CurrentThread::tid(); sem_post(&sem); func_(); }));

  sem_wait(&sem);
}

void Thread::join()
{
  joined_ = true;
  thread_->join();
}

void Thread::setDefaultName()
{
  int num = ++numCreated_;
  if (name_.empty())
  {
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "Thread%d", num);
    name_ = buf;
  }
}
