#include "hardware/LCDDisplay.h"

#include "config/Config.h"
#include "utils/Format.h"
#include "utils/Logger.h"
#include "utils/Timing.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <new>

LCDDisplay::LCDDisplay(I2CBus& bus)
    : bus_(bus), lcd_(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS) {}

bool LCDDisplay::probeAddress(uint8_t address) const {
  if (bus_.probe(address)) {
    return true;
  }
  yield();
  return bus_.probe(address);
}

uint8_t LCDDisplay::findAddress() const {
  // Solo el mapa del PCF8574 (0x20-0x27) y del PCF8574A (0x38-0x3F).
  // 0x38 es el AHT10, asi que el segundo tramo empieza en 0x39.
  Logger::warn("LCD address scan: PCF8574 0x20-0x27, PCF8574A 0x39-0x3F");
  uint8_t found = 0;
  uint8_t count = 0;
  const uint8_t first[] = {0x20, 0x39};
  const uint8_t last[] = {0x27, 0x3F};
  for (uint8_t range = 0; range < 2; ++range) {
    for (uint8_t address = first[range]; address <= last[range]; ++address) {
      if (address == LCD_I2C_ADDRESS) {
        continue;
      }
      if (probeAddress(address)) {
        Logger::warnf("LCD candidate at 0x%02X", address);
        found = address;
        ++count;
      }
      yield();
    }
  }
  if (count == 0) {
    Logger::warn("LCD scan found no other backpack address");
    return 0;
  }
  if (count > 1) {
    Logger::error("LCD scan found more than one device; set LCD_I2C_ADDRESS by hand");
    return 0;
  }
  return found;
}

bool LCDDisplay::begin() {
  present_ = false;

  // La misma secuencia que el sketch que ya muestra texto. init() llama a
  // Wire.begin(), espera y escribe. El sondeo no puede impedir ese arranque:
  // un ACK vacio justo despues de abrir el bus falla aunque el panel responda.
  lcd_.init();
  lcd_.backlight();
  lcd_.noCursor();
  lcd_.noBlink();
  lcd_.clear();

  present_ = true;
  shownState_ = DryerState::Initializing;
  shownSequence_ = 0;
  lastDrawMs_ = millis();
  lastForceMs_ = millis();
  cache_[0][0] = '\0';
  cache_[1][0] = '\0';
  setBacklight(true);
  showStatus("Starting...");

  uint8_t address = LCD_I2C_ADDRESS;
  uint8_t wire = bus_.probeResult(address);
  if (wire != 0) {
    yield();
    wire = bus_.probeResult(address);
  }
  if (wire != 0) {
    Logger::warnf("LCD probe at 0x%02X returned wire=%u", address, wire);
    const uint8_t found = findAddress();
    if (found != 0) {
      Logger::warnf(
          "Using LCD at 0x%02X for this boot; set LCD_I2C_ADDRESS in src/config/Pins.h",
          found);
      address = found;
      lcd_.~LiquidCrystal_I2C();
      new (&lcd_) LiquidCrystal_I2C(address, LCD_COLUMNS, LCD_ROWS);
      lcd_.init();
      lcd_.backlight();
      lcd_.noCursor();
      lcd_.noBlink();
      lcd_.clear();
      showStatus("Starting...");
      wire = bus_.probeResult(address);
    }
  }
  if (wire != 0) {
    Logger::warnf("LCD init sent to 0x%02X (wire=%u)", address, wire);
    return true;
  }
  Logger::infof("LCD detected at 0x%02X", address);
  return true;
}

void LCDDisplay::update(const SensorData& data, DryerState state) {
  if (!present_) {
    return;
  }

  const unsigned long now = millis();
  const bool stateChanged = state != shownState_;
  const bool newSample = data.sequence != 0 && data.sequence != shownSequence_;
  const bool periodic = timeElapsed(now, lastDrawMs_, DISPLAY_INTERVAL_MS);
  if (!stateChanged && !newSample && !periodic) {
    return;
  }

  if (timeElapsed(now, lastForceMs_, DISPLAY_REFRESH_MS)) {
    cache_[0][0] = '\0';
    cache_[1][0] = '\0';
    lastForceMs_ = now;
  }

  shownState_ = state;
  shownSequence_ = data.sequence;
  lastDrawMs_ = now;

  if (state == DryerState::Initializing) {
    showStatus("Starting...");
    return;
  }
  if (state == DryerState::SensorError || !data.valid) {
    showError(errorLine(data.status));
    return;
  }
  showSensorData(data);
}

void LCDDisplay::showSensorData(const SensorData& data) {
  if (!present_) {
    return;
  }
  if (!data.valid) {
    showError(errorLine(data.status));
    return;
  }

  char temperature[12];
  char humidity[12];
  formatFixed1(temperature, sizeof(temperature), data.temperatureC);
  formatFixed1(humidity, sizeof(humidity), data.humidityPercent);

  char line0[20];
  char line1[20];
  std::snprintf(line0, sizeof(line0), "Temp:%5s C", temperature);
  std::snprintf(line1, sizeof(line1), "Hum :%5s %%", humidity);
  writeLine(0, line0);
  writeLine(1, line1);
}

void LCDDisplay::showError(const char* message) {
  if (!present_) {
    return;
  }
  writeLine(0, "AHT10 ERROR");
  writeLine(1, message != nullptr ? message : "Check sensor");
}

void LCDDisplay::showStatus(const char* message) {
  if (!present_) {
    return;
  }
  writeLine(0, "FILAMENT DRYER");
  writeLine(1, message != nullptr ? message : "");
}

void LCDDisplay::setBacklight(bool on) {
  if (!present_) {
    return;
  }
  if (on) {
    lcd_.backlight();
  } else {
    lcd_.noBacklight();
  }
}

void LCDDisplay::writeLine(uint8_t row, const char* text) {
  if (!present_ || row >= LCD_ROWS) {
    return;
  }

  char padded[LCD_COLUMNS + 1];
  for (uint8_t column = 0; column < LCD_COLUMNS; ++column) {
    padded[column] = ' ';
  }
  padded[LCD_COLUMNS] = '\0';
  if (text != nullptr) {
    for (uint8_t column = 0; column < LCD_COLUMNS && text[column] != '\0'; ++column) {
      padded[column] = text[column];
    }
  }
  if (std::memcmp(padded, cache_[row], LCD_COLUMNS) == 0) {
    return;
  }
  std::memcpy(cache_[row], padded, LCD_COLUMNS + 1);
  lcd_.setCursor(0, row);
  lcd_.print(padded);
}

const char* LCDDisplay::errorLine(SensorStatus status) {
  switch (status) {
    case SensorStatus::OutOfRange:
      return "Out of range";
    case SensorStatus::Timeout:
      return "I2C timeout";
    case SensorStatus::BusError:
      return "I2C error";
    case SensorStatus::NotCalibrated:
      return "Not calibrated";
    case SensorStatus::Ok:
    case SensorStatus::NotInitialized:
    case SensorStatus::NotConnected:
    case SensorStatus::Invalid:
      return "Check sensor";
  }
  return "Check sensor";
}
