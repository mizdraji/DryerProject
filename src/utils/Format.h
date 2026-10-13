#pragma once

#include <cmath>
#include <cstdio>
#include <cstddef>

// Formatea un decimal con un digito sin depender de printf("%f"),
// que en algunos newlib nano no imprime flotantes.
inline void formatFixed1(char* out, size_t outSize, float value) {
  if (out == nullptr || outSize == 0) {
    return;
  }
  if (!std::isfinite(value) || value >= 1000000.0f || value <= -1000000.0f) {
    std::snprintf(out, outSize, "nan");
    return;
  }

  const bool negative = value < 0.0f;
  float magnitude = negative ? -value : value;
  int whole = static_cast<int>(magnitude);
  int tenth = static_cast<int>((magnitude - static_cast<float>(whole)) * 10.0f + 0.5f);
  if (tenth >= 10) {
    tenth = 0;
    ++whole;
  }
  if (whole == 0 && tenth == 0) {
    std::snprintf(out, outSize, "0.0");
    return;
  }
  if (negative) {
    std::snprintf(out, outSize, "-%d.%d", whole, tenth);
    return;
  }
  std::snprintf(out, outSize, "%d.%d", whole, tenth);
}
