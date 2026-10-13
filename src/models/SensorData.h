#pragma once

#include <stdint.h>

// Una lectura invalida usa NAN y valid == false.
// 0.0 es una medicion real (0 % HR o 0 C) y no significa error.
enum class SensorStatus : uint8_t {
  Ok = 0,
  NotInitialized,
  NotConnected,
  BusError,
  Timeout,
  OutOfRange,
  NotCalibrated,
  Invalid
};

struct SensorData {
  float temperatureC;
  float humidityPercent;
  bool valid;
  SensorStatus status;
  unsigned long timestampMs;
  uint32_t sequence;
};

inline const char* sensorStatusText(SensorStatus status) {
  switch (status) {
    case SensorStatus::Ok:
      return "ok";
    case SensorStatus::NotInitialized:
      return "not-initialized";
    case SensorStatus::NotConnected:
      return "not-connected";
    case SensorStatus::BusError:
      return "bus-error";
    case SensorStatus::Timeout:
      return "timeout";
    case SensorStatus::OutOfRange:
      return "out-of-range";
    case SensorStatus::NotCalibrated:
      return "not-calibrated";
    case SensorStatus::Invalid:
      return "invalid";
  }
  return "invalid";
}

SensorData makeInvalidSensorData(SensorStatus status, unsigned long timestampMs, uint32_t sequence);
