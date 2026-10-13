#include "models/SensorData.h"

#include <cmath>

SensorData makeInvalidSensorData(SensorStatus status, unsigned long timestampMs, uint32_t sequence) {
  SensorData data = {};
  data.temperatureC = nanf("");
  data.humidityPercent = nanf("");
  data.valid = false;
  data.status = status;
  data.timestampMs = timestampMs;
  data.sequence = sequence;
  return data;
}
