#include "hardware/I2CBus.h"

#include "utils/Logger.h"

#include <Arduino.h>
#include <Wire.h>

bool I2CBus::begin(uint8_t sda, uint8_t scl) {
  sda_ = sda;
  scl_ = scl;
  apply();
  Logger::infof("I2C SDA=GPIO%u SCL=GPIO%u", sda_, scl_);
  return true;
}

void I2CBus::restore() { apply(); }

void I2CBus::apply() {
  // Igual que el sketch que ya habla con este LCD: solo Wire.begin().
  // En este core eso deja 100 kHz y 150 ms de clock stretch.
  // setClock() y un stretch de 10 ms hacian que el PCF8574 y el AHT10
  // contestaran NACK (wire=2).
  Wire.begin(sda_, scl_);
}

void I2CBus::recover() {
  Logger::debug("I2C bus recovery");
  digitalWrite(scl_, HIGH);
  pinMode(scl_, OUTPUT);
  pinMode(sda_, INPUT_PULLUP);

  for (uint8_t pulse = 0; pulse < 16 && digitalRead(sda_) == LOW; ++pulse) {
    digitalWrite(scl_, LOW);
    delayMicroseconds(5);
    digitalWrite(scl_, HIGH);
    delayMicroseconds(5);
  }

  digitalWrite(sda_, LOW);
  pinMode(sda_, OUTPUT);
  delayMicroseconds(5);
  digitalWrite(scl_, HIGH);
  delayMicroseconds(5);
  digitalWrite(sda_, HIGH);
  delayMicroseconds(5);
  pinMode(sda_, INPUT_PULLUP);
  pinMode(scl_, INPUT_PULLUP);
  apply();
}

uint8_t I2CBus::probeResult(uint8_t address) const {
  Wire.beginTransmission(address);
  return Wire.endTransmission();
}

bool I2CBus::probe(uint8_t address) const {
  return probeResult(address) == 0;
}

void I2CBus::scan() const {
  Logger::warn("I2C scan started; this can disturb the AHT10");
  uint8_t found = 0;
  for (uint8_t address = 0x03; address <= 0x77; ++address) {
    if (probe(address)) {
      Logger::infof("I2C device at 0x%02X", address);
      ++found;
    }
    yield();
  }
  Logger::infof("I2C scan done, %u device(s)", found);
}
