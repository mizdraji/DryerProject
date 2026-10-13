#pragma once

#include <stdint.h>

enum class LogLevel : uint8_t {
  Debug = 0,
  Info = 1,
  Warn = 2,
  Error = 3
};

class Logger {
 public:
  static void begin(unsigned long baud);
  static void setLevel(LogLevel level);

  static void debug(const char* message);
  static void info(const char* message);
  static void warn(const char* message);
  static void error(const char* message);

  static void debugf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
  static void infof(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
  static void warnf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
  static void errorf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

 private:
  static void write(LogLevel level, const char* message);
  static bool enabled(LogLevel level);

  static LogLevel level_;
};
