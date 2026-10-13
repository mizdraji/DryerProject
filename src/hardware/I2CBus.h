#pragma once

#include <stdint.h>

// Dueno unico del periferic I2C. Los drivers no deben llamar Wire.begin()
// por su cuenta; si una libreria lo hace, hay que llamar restore() despues.
class I2CBus {
 public:
  bool begin(uint8_t sda, uint8_t scl);
  void restore();
  void recover();
  bool probe(uint8_t address) const;
  uint8_t probeResult(uint8_t address) const;
  void scan() const;

  uint8_t sda() const { return sda_; }
  uint8_t scl() const { return scl_; }

 private:
  void apply();

  uint8_t sda_ = 4;
  uint8_t scl_ = 5;
};
