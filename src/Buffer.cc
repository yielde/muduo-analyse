#include "Buffer.h"
#include <sys/uio.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd, int *saveErrno)
{
  // read multi buffers
  char extrabuf[65535] = {0}; // 64k栈空间，出作用域自动释放，是放之前已经被copy到buffer_

  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;

  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof(extrabuf);

  const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;

  const ssize_t n = ::readv(fd, vec, iovcnt);
  if (n < 0)
  {
    *saveErrno = errno;
  }
  else if (n <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}
