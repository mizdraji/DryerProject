#pragma once

// Resta sin signo: el desborde de millis() a los ~49 dias sigue siendo correcto.
inline bool timeElapsed(unsigned long now, unsigned long start, unsigned long interval) {
  return static_cast<unsigned long>(now - start) >= interval;
}
