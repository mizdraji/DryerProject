#include "utils/Logger.h"

#include <Arduino.h>
#include <cstdarg>
#include <cstdio>

LogLevel Logger::level_ = LogLevel::Info;

void Logger::begin(unsigned long baud) {
  Serial.begin(baud);
  level_ = LogLevel::Info;
}

void Logger::setLevel(LogLevel level) {
  level_ = level;
}

bool Logger::enabled(LogLevel level) {
  return static_cast<uint8_t>(level) >= static_cast<uint8_t>(level_);
}

void Logger::write(LogLevel level, const char* message) {
  if (!enabled(level) || message == nullptr) {
    return;
  }
  const char* tag = "INFO";
  switch (level) {
    case LogLevel::Debug:
      tag = "DEBUG";
      break;
    case LogLevel::Info:
      tag = "INFO";
      break;
    case LogLevel::Warn:
      tag = "WARN";
      break;
    case LogLevel::Error:
      tag = "ERROR";
      break;
  }
  Serial.printf("[%s] %s\n", tag, message);
}

void Logger::debug(const char* message) { write(LogLevel::Debug, message); }
void Logger::info(const char* message) { write(LogLevel::Info, message); }
void Logger::warn(const char* message) { write(LogLevel::Warn, message); }
void Logger::error(const char* message) { write(LogLevel::Error, message); }

void Logger::debugf(const char* fmt, ...) {
  if (!enabled(LogLevel::Debug)) {
    return;
  }
  char message[160];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  write(LogLevel::Debug, message);
}

void Logger::infof(const char* fmt, ...) {
  if (!enabled(LogLevel::Info)) {
    return;
  }
  char message[160];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  write(LogLevel::Info, message);
}

void Logger::warnf(const char* fmt, ...) {
  if (!enabled(LogLevel::Warn)) {
    return;
  }
  char message[160];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  write(LogLevel::Warn, message);
}

void Logger::errorf(const char* fmt, ...) {
  if (!enabled(LogLevel::Error)) {
    return;
  }
  char message[160];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  write(LogLevel::Error, message);
}
