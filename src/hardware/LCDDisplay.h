#pragma once

#include "config/Pins.h"
#include "hardware/I2CBus.h"
#include "models/DryerState.h"
#include "models/SensorData.h"

#include <LiquidCrystal_I2C.h>

// Pantalla HD44780 16x2 via PCF8574. La libreria no sale de esta clase.
class LCDDisplay {
 public:
  explicit LCDDisplay(I2CBus& bus);

  bool begin();
  void update(const SensorData& data, DryerState state);
  void showSensorData(const SensorData& data);
  void showError(const char* message);
  void showStatus(const char* message);
  void setBacklight(bool on);
  bool isReady() const { return present_; }

 private:
  bool probeAddress(uint8_t address) const;
  uint8_t findAddress() const;
  void writeLine(uint8_t row, const char* text);
  static const char* errorLine(SensorStatus status);

  I2CBus& bus_;
  LiquidCrystal_I2C lcd_;
  char cache_[2][LCD_COLUMNS + 1] = {};
  DryerState shownState_ = DryerState::Initializing;
  uint32_t shownSequence_ = 0;
  unsigned long lastDrawMs_ = 0;
  unsigned long lastForceMs_ = 0;
  bool present_ = false;
};
