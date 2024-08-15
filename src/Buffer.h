#pragma once
#include "copyable.h"
#include <vector>
#include <string>
#include <algorithm>

class Buffer : public copyable
{

  /// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
  ///
  /// @code
  /// +-------------------+------------------+------------------+
  /// | prependable bytes |  readable bytes  |  writable bytes  |
  /// |                   |     (CONTENT)    |                  |
  /// +-------------------+------------------+------------------+
  /// |                   |                  |                  |
  /// 0      <=      readerIndex   <=   writerIndex    <=     size
  /// @endcode

public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
  {
  }
  size_t prependableBytes() const { return readerIndex_; }
  size_t readableBytes() const { return writerIndex_ - readerIndex_; }
  size_t writableBytes() const { return buffer_.size() - writerIndex_; }

  const char *peek() const
  {
    // Buffer可读区域起始地址
    return begin() + readerIndex_;
  }

  void retrieve(size_t len)
  {
    if (len < readableBytes())
    {
      readerIndex_ += len;
    }
    else
    {
      retrieveAll();
    }
  }

  void retrieveAll()
  {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  std::string retrieveAllAsString()
  {
    return retrieveAsString(readableBytes());
  }

  std::string retrieveAsString(size_t len)
  {
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  // write
  void ensureWritableBytes(size_t len)
  {
    if (writableBytes() < len)
    {
      makeSpace(len);
    }
  }

  void append(const char *data, size_t len)
  {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }

  char *beginWrite()
  {
    return begin() + writerIndex_;
  }

  const char *beginWrite() const
  {
    return begin() + writerIndex_;
  }

  ssize_t readFd(int fd, int *saveErrno);

private:
  char *begin()
  {
    return &*buffer_.begin();
  }
  const char *begin() const
  {
    return &*buffer_.begin();
  }

  void makeSpace(size_t len)
  {
    if (writableBytes() + prependableBytes() // 空开了readableBytes
        < len + kCheapPrepend)
    {
      buffer_.resize(writerIndex_ + len); // 扩容writable长度到len
    }
    else
    {
      // 读过的readable bytes + writable bytes够用
      size_t readable = readableBytes();
      // 把未读的数据部分复制到kCheapPrepend后面，剩余的所有空间给write做缓冲区
      std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

  std::vector<char> buffer_; // 使用vector的动态扩容
  size_t readerIndex_;
  size_t writerIndex_;
};