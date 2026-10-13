#pragma once

#include <stdint.h>

// ESP-12F: GPIO4 y GPIO5 no son pines de arranque.
// En una placa tipo NodeMCU los mismos pines se serigrafian D2 (SDA) y D1 (SCL).
// El modulo desnudo no lleva esa serigrafia: cablear por numero de GPIO.
constexpr uint8_t I2C_SDA_PIN = 4;
constexpr uint8_t I2C_SCL_PIN = 5;

// AHT10: direccion fija 7 bit. Este LCD es un PCF8574 en 0x27.
// Un PCF8574A suele estar en 0x3F; cambiar solo esta constante.
constexpr uint8_t AHT10_I2C_ADDRESS = 0x38;
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;

constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;
