#include "hardware/AHT10Sensor.h"

#include "config/Config.h"
#include "config/Pins.h"
#include "hardware/Aht10Decoder.h"
#include "utils/Format.h"
#include "utils/Logger.h"
#include "utils/Timing.h" 

#include <Wire.h>

namespace {
constexpr uint8_t kSoftReset[] = {0xBA};
constexpr uint8_t kInitialize[] = {0xE1, 0x08, 0x00};
constexpr uint8_t kMeasure[] = {0xAC, 0x33, 0x00};
}

AHT10Sensor::AHT10Sensor(I2CBus& bus)
    : bus_(bus),
      data_(makeInvalidSensorData(SensorStatus::NotInitialized, 0, 0)) {}

bool AHT10Sensor::begin() {
  begun_ = true;
  ready_ = false;
  connected_ = false;
  announced_ = false;
  failureLogged_ = false;
  softResetTries_ = 0;
  sequence_ = 0;
  data_ = makeInvalidSensorData(SensorStatus::NotInitialized, 0, 0);
  enter(Phase::StartupWait, millis());
  return true;
}

void AHT10Sensor::update() {
  if (!begun_) {
    return;
  }

  const unsigned long now = millis();
  switch (phase_) {
    case Phase::StartupWait:
      if (timeElapsed(now, phaseStartedMs_, SENSOR_STARTUP_DELAY_MS)) {
        enter(Phase::SoftReset, now);
      }
      break;

    case Phase::SoftReset: {
      uint8_t wireResult = 0;
      if (writeRaw(kSoftReset, sizeof(kSoftReset), wireResult)) {
        softResetTries_ = 0;
        enter(Phase::ResetWait, now);
        break;
      }
      ++softResetTries_;
      if (softResetTries_ < SENSOR_SOFT_RESET_TRIES) {
        enter(Phase::ResetRetryWait, now);
        break;
      }
      fail(statusFromWire(wireResult), now);
      break;
    }

    case Phase::ResetRetryWait:
      if (timeElapsed(now, phaseStartedMs_, SENSOR_RESET_DELAY_MS)) {
        enter(Phase::SoftReset, now);
      }
      break;

    case Phase::ResetWait:
      if (timeElapsed(now, phaseStartedMs_, SENSOR_RESET_DELAY_MS)) {
        enter(Phase::Initialize, now);
      }
      break;

    case Phase::Initialize: {
      uint8_t wireResult = 0;
      if (!writeRaw(kInitialize, sizeof(kInitialize), wireResult)) {
        fail(statusFromWire(wireResult), now);
        break;
      }
      enter(Phase::InitWait, now);
      break;
    }

    case Phase::InitWait:
      if (timeElapsed(now, phaseStartedMs_, SENSOR_INIT_DELAY_MS)) {
        startMeasurement(now);
      }
      break;

    case Phase::MeasureWait: {
      if (!timeElapsed(now, phaseStartedMs_, SENSOR_MEASURE_DELAY_MS)) {
        break;
      }
      if (lastAttemptMs_ != 0 && !timeElapsed(now, lastAttemptMs_, SENSOR_BUSY_POLL_MS)) {
        break;
      }
      lastAttemptMs_ = now;

      uint8_t raw[6] = {};
      if (!readRaw(raw, sizeof(raw))) {
        fail(SensorStatus::BusError, now);
        break;
      }
      const Aht10Decoded decoded = decodeAht10(raw);
      if (decoded.status == Aht10Status::Busy) {
        if (timeElapsed(now, phaseStartedMs_, SENSOR_MEASURE_TIMEOUT_MS)) {
          fail(SensorStatus::Timeout, now);
        }
        break;
      }
      if (decoded.status == Aht10Status::NotCalibrated) {
        fail(SensorStatus::NotCalibrated, now);
        break;
      }
      if (decoded.status == Aht10Status::OutOfRange) {
        char temperature[16];
        char humidity[16];
        formatFixed1(temperature, sizeof(temperature), decoded.temperatureC);
        formatFixed1(humidity, sizeof(humidity), decoded.humidityPercent);
        Logger::debugf("AHT10 out of range T=%s RH=%s", temperature, humidity);
        fail(SensorStatus::OutOfRange, now);
        break;
      }
      succeed(decoded.temperatureC, decoded.humidityPercent, now);
      break;
    }

    case Phase::Idle:
      if (timeElapsed(now, phaseStartedMs_, SENSOR_INTERVAL_MS)) {
        startMeasurement(now);
      }
      break;

    case Phase::ErrorBackoff:
      if (timeElapsed(now, phaseStartedMs_, SENSOR_RETRY_INTERVAL_MS)) {
        softResetTries_ = 0;
        enter(Phase::SoftReset, now);
      }
      break;
  }
}

bool AHT10Sensor::writeRaw(const uint8_t* data, size_t length, uint8_t& wireResult) {
  Wire.beginTransmission(AHT10_I2C_ADDRESS);
  if (data != nullptr && length > 0) {
    Wire.write(data, length);
  }
  wireResult = Wire.endTransmission();
  return wireResult == 0;
}

bool AHT10Sensor::readRaw(uint8_t* data, uint8_t length) {
  const uint8_t received = Wire.requestFrom(AHT10_I2C_ADDRESS, length);
  if (received != length || data == nullptr) {
    return false;
  }
  for (uint8_t index = 0; index < length; ++index) {
    data[index] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

bool AHT10Sensor::startMeasurement(unsigned long now) {
  uint8_t wireResult = 0;
  if (!writeRaw(kMeasure, sizeof(kMeasure), wireResult)) {
    fail(statusFromWire(wireResult), now);
    return false;
  }
  lastAttemptMs_ = 0;
  enter(Phase::MeasureWait, now);
  return true;
}

void AHT10Sensor::succeed(float temperatureC, float humidityPercent, unsigned long now) {
  SensorData sample = {};
  sample.temperatureC = temperatureC;
  sample.humidityPercent = humidityPercent;
  sample.valid = true;
  sample.status = SensorStatus::Ok;
  sample.timestampMs = now;
  sample.sequence = nextSequence();
  data_ = sample;
  connected_ = true;
  ready_ = true;

  if (!announced_) {
    Logger::infof("AHT10 detected at 0x%02X", AHT10_I2C_ADDRESS);
    announced_ = true;
  } else if (failureLogged_) {
    Logger::info("AHT10 reading resumed");
  }
  failureLogged_ = false;

  char temperature[16];
  char humidity[16];
  formatFixed1(temperature, sizeof(temperature), temperatureC);
  formatFixed1(humidity, sizeof(humidity), humidityPercent);
  Logger::debugf("AHT10 T=%s C RH=%s %%", temperature, humidity);
  enter(Phase::Idle, now);
}

void AHT10Sensor::fail(SensorStatus status, unsigned long now) {
  if (status == SensorStatus::NotConnected) {
    connected_ = false;
  }
  if (status == SensorStatus::NotConnected || status == SensorStatus::BusError ||
      status == SensorStatus::Timeout) {
    bus_.recover();
  }

  data_ = makeInvalidSensorData(status, now, nextSequence());
  ready_ = true;
  if (!failureLogged_) {
    Logger::warnf("AHT10 error: %s", sensorStatusText(status));
    failureLogged_ = true;
  } else {
    Logger::debugf("AHT10 error: %s", sensorStatusText(status));
  }
  enter(Phase::ErrorBackoff, now);
}

void AHT10Sensor::enter(Phase phase, unsigned long now) {
  phase_ = phase;
  phaseStartedMs_ = now;
}

uint32_t AHT10Sensor::nextSequence() {
  ++sequence_;
  if (sequence_ == 0) {
    sequence_ = 1;
  }
  return sequence_;
}

SensorStatus AHT10Sensor::statusFromWire(uint8_t wireResult) {
  if (wireResult == 2) {
    return SensorStatus::NotConnected;
  }
  if (wireResult == 5) {
    return SensorStatus::Timeout;
  }
  return SensorStatus::BusError;
}
