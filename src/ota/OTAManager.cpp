#include "ota/OTAManager.h"

#include "config/Config.h"
#include "utils/Logger.h"

#include <ArduinoOTA.h>
#include <cstring>

bool OTAManager::begin(const char* hostname, const char* password) {
  if (updating_) {
    return false;
  }
  if (password == nullptr || password[0] == '\0' || std::strcmp(password, "YOUR_OTA_PASSWORD") == 0) {
    passwordRejected_ = true;
    ready_ = false;
    Logger::error("OTA disabled: set OTA_PASSWORD in include/secrets.h");
    return false;
  }
  if (hostname == nullptr || hostname[0] == '\0') {
    ready_ = false;
    Logger::error("OTA disabled: hostname is empty");
    return false;
  }

  passwordRejected_ = false;
  progressStep_ = 0;

  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setPassword(password);
  ArduinoOTA.setRebootOnSuccess(true);

  ArduinoOTA.onStart([this]() {
    updating_ = true;
    progressStep_ = 0;
    Logger::info("OTA START");
  });
  ArduinoOTA.onEnd([this]() {
    updating_ = false;
    Logger::info("OTA END");
  });
  ArduinoOTA.onProgress([this](unsigned int current, unsigned int total) {
    if (total == 0) {
      return;
    }
    const unsigned int percent = (current * 100U) / total;
    const unsigned int step = percent / 25U;
    if (step == 0 || step >= 4U || step == progressStep_) {
      return;
    }
    progressStep_ = step;
    Logger::infof("OTA progress: %u%%", step * 25U);
  });
  ArduinoOTA.onError([this](ota_error_t error) {
    updating_ = false;
    Logger::errorf("OTA ERROR: %s", errorName(static_cast<int>(error)));
  });

  ArduinoOTA.begin();
  started_ = true;
  ready_ = true;
  unavailableLogged_ = false;
  Logger::info("OTA READY");
  return true;
}

void OTAManager::update() {
  if (started_ && (ready_ || updating_)) {
    ArduinoOTA.handle();
  }
}

void OTAManager::suspend() {
  if (updating_) {
    return;
  }
  ready_ = false;
  // begin() no hace nada si el servicio ya estaba iniciado. end() existe en
  // el core 3.1.2 y libera el UDP y mDNS para poder arrancar de nuevo.
  if (started_) {
    ArduinoOTA.end();
    started_ = false;
  }
  if (unavailableLogged_) {
    return;
  }
  unavailableLogged_ = true;
  Logger::warn("OTA not available");
}

const char* OTAManager::errorName(int error) {
  switch (error) {
    case OTA_AUTH_ERROR:
      return "auth";
    case OTA_BEGIN_ERROR:
      return "begin";
    case OTA_CONNECT_ERROR:
      return "connect";
    case OTA_RECEIVE_ERROR:
      return "receive";
    case OTA_END_ERROR:
      return "end";
    default:
      return "unknown";
  }
}
