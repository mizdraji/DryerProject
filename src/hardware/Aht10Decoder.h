#pragma once

#include <stdint.h>

// Conversion ASAIR del AHT10:
//   HR% = raw20 / 2^20 * 100
//   T C = raw20 / 2^20 * 200 - 50
// Limites de la hoja de datos: -40..85 C y 0..100 % HR.

constexpr uint8_t AHT10_STATUS_BUSY = 0x80;
constexpr uint8_t AHT10_STATUS_CALIBRATED = 0x08;
constexpr float AHT10_TEMP_MIN_C = -40.0f;
constexpr float AHT10_TEMP_MAX_C = 85.0f;

enum class Aht10Status : uint8_t {
  Ok = 0,
  Busy,
  NotCalibrated,
  OutOfRange
};

struct Aht10Decoded {
  float temperatureC;
  float humidityPercent;
  Aht10Status status;
};

inline Aht10Decoded decodeAht10(const uint8_t data[6]) {
  Aht10Decoded decoded = {};
  decoded.temperatureC = 0.0f;
  decoded.humidityPercent = 0.0f;
  decoded.status = Aht10Status::OutOfRange;

  if (data == nullptr) {
    return decoded;
  }

  const uint32_t humidityRaw =
      (static_cast<uint32_t>(data[1]) << 12) |
      (static_cast<uint32_t>(data[2]) << 4) |
      (static_cast<uint32_t>(data[3]) >> 4);
  const uint32_t temperatureRaw =
      (static_cast<uint32_t>(data[3] & 0x0F) << 16) |
      (static_cast<uint32_t>(data[4]) << 8) |
      static_cast<uint32_t>(data[5]);

  decoded.humidityPercent = (static_cast<float>(humidityRaw) * 100.0f) / 1048576.0f;
  decoded.temperatureC = (static_cast<float>(temperatureRaw) * 200.0f) / 1048576.0f - 50.0f;

  if ((data[0] & AHT10_STATUS_BUSY) != 0) {
    decoded.status = Aht10Status::Busy;
    return decoded;
  }
  if ((data[0] & AHT10_STATUS_CALIBRATED) == 0) {
    decoded.status = Aht10Status::NotCalibrated;
    return decoded;
  }
  if (decoded.temperatureC < AHT10_TEMP_MIN_C || decoded.temperatureC > AHT10_TEMP_MAX_C ||
      decoded.humidityPercent < 0.0f || decoded.humidityPercent > 100.0f) {
    decoded.status = Aht10Status::OutOfRange;
    return decoded;
  }

  decoded.status = Aht10Status::Ok;
  return decoded;
}
