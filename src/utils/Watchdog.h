#pragma once

// Observa el motivo de reinicio. No desactiva ni reconfigura el watchdog:
// el core Arduino ya alimenta el WDT al salir de loop() y dentro de yield().
class Watchdog {
 public:
  static void begin();
  static const char* resetReason();
  static bool unexpectedReset();

 private:
  static const char* reason_;
  static bool unexpected_;
};
