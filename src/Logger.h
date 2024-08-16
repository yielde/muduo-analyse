#pragma once

#include <string>

#include "nocopyable.h"

#define LOG_INFO(logmsgFormat, ...)                   \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    logger.setLogLevel(INFO);                         \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);

#define LOG_ERROR(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    logger.setLogLevel(ERROR);                        \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);

#define LOG_FATAL(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    logger.setLogLevel(FATAL);                        \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                \
  do                                                \
  {                                                 \
    Logger &logger = Logger::instance();            \
    logger.setLogLevel(DEBUG);                      \
    char buf[1024] = {0};                           \
    snprintf(buf, 1024, logmsgFormat, ##_VA_ARGS_); \
    logger.log(buf);                                \
  } while (0);

#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

enum LogLevel
{
  INFO,
  ERROR,
  FATAL,
  DEBUG
};

class Logger : nocopyable
{
public:
  // 单例模式的logger，线程安全
  static Logger &instance();
  void setLogLevel(int level);
  void log(std::string msg);

private:
  Logger() {}

private:
  int logLevel_;
};