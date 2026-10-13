#pragma once

#include "control/Actuator.h"
#include "models/DryerState.h"
#include "models/SensorData.h"

// Lazo local del secador. En V0.1 solo supervisa el sensor.
// No conoce pines ni ArduinoOTA.
class DryerController {
 public:
  explicit DryerController(Actuator* actuator = nullptr);

  void begin();
  void update(bool sensorReady, const SensorData& latest);
  DryerState getState() const { return state_; }
  const SensorData& getData() const { return published_; }

  // V0.1 no tiene actuador, asi que el mantenimiento siempre esta permitido.
  // Con un actuador futuro, devuelve false mientras ese actuador este habilitado.
  bool isMaintenanceAllowed() const;
  void forceSafeActuatorState();

 private:
  Actuator* actuator_;
  SensorData published_;
  DryerState state_ = DryerState::Initializing;
  uint8_t failures_ = 0;
  uint32_t lastSequence_ = 0;
  bool safeStateLogged_ = false;
};
