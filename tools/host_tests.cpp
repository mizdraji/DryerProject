#include "hardware/Aht10Decoder.h"
#include "utils/Format.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::printf("FAIL %s\n", message);
    ++failures;
  }
}

void expectText(const char* actual, const char* expected, const char* message) {
  if (std::strcmp(actual, expected) != 0) {
    std::printf("FAIL %s (got '%s', expected '%s')\n", message, actual, expected);
    ++failures;
  }
}

void pack(uint8_t out[6], uint8_t status, uint32_t humidityRaw, uint32_t temperatureRaw) {
  out[0] = status;
  out[1] = static_cast<uint8_t>((humidityRaw >> 12) & 0xFF);
  out[2] = static_cast<uint8_t>((humidityRaw >> 4) & 0xFF);
  out[3] = static_cast<uint8_t>(((humidityRaw & 0x0F) << 4) | ((temperatureRaw >> 16) & 0x0F));
  out[4] = static_cast<uint8_t>((temperatureRaw >> 8) & 0xFF);
  out[5] = static_cast<uint8_t>(temperatureRaw & 0xFF);
}

void testKnownPoint() {
  // 50 % HR -> raw 524288. 25 C -> raw 393216. Estado calibrado e idle: 0x08.
  uint8_t raw[6] = {};
  pack(raw, 0x08, 524288u, 393216u);
  const Aht10Decoded decoded = decodeAht10(raw);
  expect(decoded.status == Aht10Status::Ok, "known point status");
  expect(std::fabs(decoded.humidityPercent - 50.0f) < 0.001f, "known point humidity");
  expect(std::fabs(decoded.temperatureC - 25.0f) < 0.001f, "known point temperature");
}

void testReferenceHalfScale() {
  // raw 524288 en ambos canales: 50 % HR y 50 C.
  const uint8_t raw[6] = {0x08, 0x80, 0x00, 0x08, 0x00, 0x00};
  const Aht10Decoded decoded = decodeAht10(raw);
  expect(decoded.status == Aht10Status::Ok, "half scale status");
  expect(std::fabs(decoded.humidityPercent - 50.0f) < 0.001f, "half scale humidity");
  expect(std::fabs(decoded.temperatureC - 50.0f) < 0.001f, "half scale temperature");
}

void testBusyAndUncalibrated() {
  uint8_t raw[6] = {};
  pack(raw, 0x88, 524288u, 393216u);
  expect(decodeAht10(raw).status == Aht10Status::Busy, "busy wins over data");

  pack(raw, 0xFF, 524288u, 393216u);
  expect(decodeAht10(raw).status == Aht10Status::Busy, "0xFF is busy");

  pack(raw, 0x00, 524288u, 393216u);
  expect(decodeAht10(raw).status == Aht10Status::NotCalibrated, "not calibrated");
}

void testOutOfRange() {
  uint8_t raw[6] = {};
  pack(raw, 0x08, 0u, 0u);
  const Aht10Decoded cold = decodeAht10(raw);
  expect(cold.status == Aht10Status::OutOfRange, "raw zero is out of range");
  expect(cold.temperatureC < AHT10_TEMP_MIN_C, "raw zero temperature is below spec");

  pack(raw, 0x08, 0xFFFFFu, 0xFFFFFu);
  expect(decodeAht10(raw).status == Aht10Status::OutOfRange, "raw max is out of range");
}

void testLimitsAreValid() {
  // -40 C -> raw = 0.05 * 2^20 = 52428.8; 52429 sigue dentro de rango.
  // 85 C -> raw = 0.675 * 2^20 = 707788.8; 707788 sigue dentro de rango.
  uint8_t raw[6] = {};
  pack(raw, 0x08, 0u, 52429u);
  const Aht10Decoded cold = decodeAht10(raw);
  expect(cold.status == Aht10Status::Ok, "lower temperature limit");
  expect(cold.temperatureC >= AHT10_TEMP_MIN_C, "lower temperature not rejected");

  pack(raw, 0x08, 0xFFFFFu, 707788u);
  const Aht10Decoded hot = decodeAht10(raw);
  expect(hot.status == Aht10Status::Ok, "upper temperature limit");
  expect(hot.temperatureC <= AHT10_TEMP_MAX_C, "upper temperature not rejected");
}

void testFormat() {
  char text[16];
  formatFixed1(text, sizeof(text), 25.4f);
  expectText(text, "25.4", "25.4");
  formatFixed1(text, sizeof(text), -40.0f);
  expectText(text, "-40.0", "-40");
  formatFixed1(text, sizeof(text), -9.5f);
  expectText(text, "-9.5", "-9.5");
  formatFixed1(text, sizeof(text), 9.96f);
  expectText(text, "10.0", "rounding");
  formatFixed1(text, sizeof(text), -0.04f);
  expectText(text, "0.0", "negative near zero");
  formatFixed1(text, sizeof(text), nanf(""));
  expectText(text, "nan", "nan");
}

}  // namespace

int main() {
  testKnownPoint();
  testReferenceHalfScale();
  testBusyAndUncalibrated();
  testOutOfRange();
  testLimitsAreValid();
  testFormat();
  if (failures != 0) {
    std::printf("%d failure(s)\n", failures);
    return 1;
  }
  std::printf("host tests passed\n");
  return 0;
}
