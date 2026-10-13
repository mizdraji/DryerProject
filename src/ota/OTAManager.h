#pragma once

// Servicio de mantenimiento. El secador no depende de el para medir ni mostrar.
// Toda llamada a ArduinoOTA queda dentro de esta clase.
class OTAManager {
 public:
  bool begin(const char* hostname, const char* password);
  void update();
  void suspend();

  bool isReady() const { return ready_; }
  bool isUpdating() const { return updating_; }
  bool passwordRejected() const { return passwordRejected_; }

 private:
  static const char* errorName(int error);

  bool started_ = false;
  bool ready_ = false;
  bool updating_ = false;
  bool passwordRejected_ = false;
  bool unavailableLogged_ = false;
  unsigned int progressStep_ = 0;
};
