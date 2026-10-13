#include "control/DryerController.h"

#include "config/Config.h"
#include "utils/Logger.h"

DryerController::DryerController(Actuator* actuator)
    : actuator_(actuator),
      published_(makeInvalidSensorData(SensorStatus::NotInitialized, 0, 0)) {}

void DryerController::begin() {
  forceSafeActuatorState();
  state_ = DryerState::Initializing;
  failures_ = 0;
  lastSequence_ = 0;
  published_ = makeInvalidSensorData(SensorStatus::NotInitialized, 0, 0);
}

void DryerController::update(bool sensorReady, const SensorData& latest) {
  if (!sensorReady) {
    state_ = DryerState::Initializing;
    return;
  }
  if (latest.sequence == 0 || latest.sequence == lastSequence_) {
    return;
  }

  lastSequence_ = latest.sequence;
  if (latest.valid) {
    published_ = latest;
    failures_ = 0;
    state_ = DryerState::Idle;
    return;
  }

  if (failures_ < 255) {
    ++failures_;
  }
  if (published_.valid && failures_ < SENSOR_MAX_FAILURES) {
    state_ = DryerState::Idle;
    return;
  }

  published_ = latest;
  state_ = DryerState::SensorError;
}

bool DryerController::isMaintenanceAllowed() const {
  return actuator_ == nullptr || !actuator_->isEnabled();
}

void DryerController::forceSafeActuatorState() {
  if (actuator_ != nullptr) {
    actuator_->forceOff();
  }
  if (safeStateLogged_) {
    return;
  }
  safeStateLogged_ = true;
  if (actuator_ == nullptr) {
    Logger::info("Safe state applied: no power actuators in V0.1");
    return;
  }
  Logger::info("Safe state applied: actuator forced OFF");
}
