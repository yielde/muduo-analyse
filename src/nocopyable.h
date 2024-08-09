#pragma once

/*
派生类不可拷贝构造和赋值
*/

class nocopyable
{
public:
  nocopyable(const nocopyable &) = delete;
  nocopyable &operator=(const nocopyable &) = delete;

protected:
  nocopyable() = default;
  ~nocopyable() = default;
};