#include "utils/Watchdog.h"

#include "utils/Logger.h"

#include <Esp.h>

extern "C" {
#include <user_interface.h>
}

const char* Watchdog::reason_ = "Unknown";
bool Watchdog::unexpected_ = false;

void Watchdog::begin() {
  const rst_info* info = ESP.getResetInfoPtr();
  if (info == nullptr) {
    reason_ = "Unknown";
    unexpected_ = true;
    Logger::error("Unexpected reset: reset info unavailable");
    return;
  }

  switch (info->reason) {
    case REASON_DEFAULT_RST:
      reason_ = "Power On";
      unexpected_ = false;
      break;
    case REASON_WDT_RST:
      reason_ = "Hardware Watchdog";
      unexpected_ = true;
      break;
    case REASON_EXCEPTION_RST:
      reason_ = "Exception";
      unexpected_ = true;
      Logger::errorf("Unexpected reset: Exception cause=%u epc1=0x%08lx",
                     static_cast<unsigned>(info->exccause),
                     static_cast<unsigned long>(info->epc1));
      return;
    case REASON_SOFT_WDT_RST:
      reason_ = "Software Watchdog";
      unexpected_ = true;
      break;
    case REASON_SOFT_RESTART:
      // Tambien es el reinicio normal despues de un OTA correcto.
      // No hay un flag persistente que distinga ese caso en V0.1.
      reason_ = "Software/System restart";
      unexpected_ = false;
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      reason_ = "Deep-Sleep Wake";
      unexpected_ = false;
      break;
    case REASON_EXT_SYS_RST:
      reason_ = "External System";
      unexpected_ = false;
      break;
    default:
      reason_ = "Unknown";
      unexpected_ = true;
      Logger::errorf("Unexpected reset: unknown code %lu",
                     static_cast<unsigned long>(info->reason));
      return;
  }

  if (unexpected_) {
    Logger::errorf("Unexpected reset: %s", reason_);
    return;
  }
  Logger::infof("Reset reason: %s", reason_);
}

const char* Watchdog::resetReason() { return reason_; }

bool Watchdog::unexpectedReset() { return unexpected_; }
