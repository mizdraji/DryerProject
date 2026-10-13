#include <Arduino.h>
#include <Esp.h>

#include "config/Config.h"
#include "config/Pins.h"
#include "config/Secrets.h"
#include "control/DryerController.h"
#include "hardware/AHT10Sensor.h"
#include "hardware/I2CBus.h"
#include "hardware/LCDDisplay.h"
#include "network/NetworkManager.h"
#include "ota/OTAManager.h"
#include "utils/Logger.h"
#include "utils/Watchdog.h"

namespace {

I2CBus i2cBus;
AHT10Sensor sensor(i2cBus);
LCDDisplay display(i2cBus);
NetworkManager network;
OTAManager ota;
DryerController dryer;

void serviceMaintenance() {
  const bool canStartOta = network.isConnected() && dryer.isMaintenanceAllowed() &&
                           !ota.passwordRejected();
  if (canStartOta) {
    if (!ota.isReady() && !ota.isUpdating()) {
      ota.begin(network.hostname(), OTA_PASSWORD);
    }
  } else if (!ota.isUpdating() &&
             (network.isDisabled() || network.hasConnectFailed() || ota.isReady() ||
              !dryer.isMaintenanceAllowed())) {
    ota.suspend();
  }

  ota.update();
}

}  // namespace

void setup() {
  Logger::begin(SERIAL_BAUD);
  Logger::infof("Firmware version: %s", FIRMWARE_VERSION);
  Watchdog::begin();

  dryer.forceSafeActuatorState();

  i2cBus.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (I2C_SCAN_ON_BOOT) {
    i2cBus.scan();
  }

  display.begin();
  // LiquidCrystal_I2C::init() vuelve a llamar Wire.begin() sin pines.
  // Restauramos GPIO4 y GPIO5 despues de esa llamada.
  i2cBus.restore();

  sensor.begin();
  dryer.begin();
  network.begin(WIFI_SSID, WIFI_PASSWORD);

  Logger::infof("Free heap: %u bytes", static_cast<unsigned>(ESP.getFreeHeap()));
}

void loop() {
  network.update();
  serviceMaintenance();

  if (!ota.isUpdating()) {
    sensor.update();
    dryer.update(sensor.isReady(), sensor.getData());
    display.update(dryer.getData(), dryer.getState());
  }
}
