#pragma once

#include <stdint.h>

// Unica fuente de verdad de la version. Versionado semantico MAJOR.MINOR.PATCH.
constexpr char FIRMWARE_VERSION[] = "0.1.0";
constexpr char HOSTNAME_PREFIX[] = "FILAMENT-DRYER-";

constexpr unsigned long SERIAL_BAUD = 115200;

// AHT10. Los tiempos salen de la hoja ASAIR: >20 ms tras reset, >75 ms de medida.
constexpr unsigned long SENSOR_STARTUP_DELAY_MS = 40;
constexpr unsigned long SENSOR_RESET_DELAY_MS = 20;
constexpr unsigned long SENSOR_INIT_DELAY_MS = 10;
constexpr unsigned long SENSOR_MEASURE_DELAY_MS = 80;
constexpr unsigned long SENSOR_MEASURE_TIMEOUT_MS = 200;
constexpr unsigned long SENSOR_BUSY_POLL_MS = 20;
constexpr unsigned long SENSOR_INTERVAL_MS = 2000;
constexpr unsigned long SENSOR_RETRY_INTERVAL_MS = 5000;
constexpr uint8_t SENSOR_MAX_FAILURES = 3;
constexpr uint8_t SENSOR_SOFT_RESET_TRIES = 3;

constexpr unsigned long DISPLAY_INTERVAL_MS = 1000;
constexpr unsigned long DISPLAY_REFRESH_MS = 10000;

constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10000;

// El barrido completo puede confundir al AHT10. Dejarlo en false en uso normal.
constexpr bool I2C_SCAN_ON_BOOT = false;

constexpr uint16_t OTA_PORT = 8266;

constexpr uint8_t WIFI_SSID_MAX_LENGTH = 32;
constexpr uint8_t WIFI_PASSWORD_MAX_LENGTH = 63;
