#pragma once

#include "hardware/I2CBus.h"
#include "models/SensorData.h"

#include <stddef.h>
#include <stdint.h>

// Driver del AHT10. El resto del firmware no habla I2C con el sensor.
// begin() solo arma la maquina de estados; la deteccion ocurre en update().
class AHT10Sensor {
 public:
  explicit AHT10Sensor(I2CBus& bus);

  bool begin();
  void update();
  SensorData getData() const { return data_; }
  bool isConnected() const { return connected_; }
  bool isReady() const { return ready_; }

 private:
  enum class Phase : uint8_t {
    StartupWait,
    SoftReset,
    ResetRetryWait,
    ResetWait,
    Initialize,
    InitWait,
    MeasureWait,
    Idle,
    ErrorBackoff
  };

  bool writeRaw(const uint8_t* data, size_t length, uint8_t& wireResult);
  bool readRaw(uint8_t* data, uint8_t length);
  bool startMeasurement(unsigned long now);
  void succeed(float temperatureC, float humidityPercent, unsigned long now);
  void fail(SensorStatus status, unsigned long now);
  void enter(Phase phase, unsigned long now);
  uint32_t nextSequence();
  static SensorStatus statusFromWire(uint8_t wireResult);

  I2CBus& bus_;
  SensorData data_;
  Phase phase_ = Phase::StartupWait;
  unsigned long phaseStartedMs_ = 0;
  unsigned long lastAttemptMs_ = 0;
  uint32_t sequence_ = 0;
  uint8_t softResetTries_ = 0;
  bool begun_ = false;
  bool ready_ = false;
  bool connected_ = false;
  bool announced_ = false;
  bool failureLogged_ = false;
};
