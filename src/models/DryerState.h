#pragma once

#include <stdint.h>

// V0.1 no calienta. Estos estados solo describen el lazo local de medicion.
enum class DryerState : uint8_t {
  Initializing = 0,
  Idle,
  SensorError
};
