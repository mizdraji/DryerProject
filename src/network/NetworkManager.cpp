#include "network/NetworkManager.h"

#include "config/Config.h"
#include "utils/Logger.h"
#include "utils/Timing.h"

#include <ESP8266WiFi.h>
#include <cstdio>
#include <cstring>

bool NetworkManager::begin(const char* ssid, const char* password) {
  ssid_ = ssid != nullptr ? ssid : "";
  password_ = password != nullptr ? password : "";

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  buildHostname();
  WiFi.hostname(hostname_);
  Logger::infof("Hostname: %s", hostname_);

  if (!credentialsAccepted(ssid_, password_)) {
    WiFi.mode(WIFI_OFF);
    state_ = LinkState::Disabled;
    Logger::warn("WiFi disabled, local control continues");
    return false;
  }

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  Logger::info("WiFi connecting");
  WiFi.begin(ssid_, password_);
  state_ = LinkState::Connecting;
  attemptStartedMs_ = millis();
  return true;
}

void NetworkManager::update() {
  if (state_ == LinkState::Disabled) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (state_ != LinkState::Connected) {
      state_ = LinkState::Connected;
      const IPAddress ip = WiFi.localIP();
      Logger::infof("IP: %u.%u.%u.%u",
                    static_cast<unsigned>(ip[0]),
                    static_cast<unsigned>(ip[1]),
                    static_cast<unsigned>(ip[2]),
                    static_cast<unsigned>(ip[3]));
      Logger::debugf("RSSI: %d dBm", WiFi.RSSI());
    }
    return;
  }

  const unsigned long now = millis();
  if (state_ == LinkState::Connected) {
    state_ = LinkState::Connecting;
    attemptStartedMs_ = now;
    Logger::warn("WiFi lost, local control continues");
    return;
  }

  if (state_ == LinkState::Connecting) {
    if (timeElapsed(now, attemptStartedMs_, WIFI_CONNECT_TIMEOUT_MS)) {
      state_ = LinkState::WaitingRetry;
      retryStartedMs_ = now;
      Logger::warn("WiFi timeout, local control continues");
    }
    return;
  }

  if (state_ == LinkState::WaitingRetry &&
      timeElapsed(now, retryStartedMs_, WIFI_RETRY_INTERVAL_MS)) {
    reconnect();
  }
}

void NetworkManager::reconnect() {
  if (state_ == LinkState::Disabled || ssid_ == nullptr || ssid_[0] == '\0') {
    return;
  }
  Logger::info("WiFi retry");
  WiFi.begin(ssid_, password_);
  state_ = LinkState::Connecting;
  attemptStartedMs_ = millis();
}

IPAddress NetworkManager::getIP() const {
  if (!isConnected()) {
    return IPAddress(0, 0, 0, 0);
  }
  return WiFi.localIP();
}

void NetworkManager::buildHostname() {
  uint8_t mac[6] = {};
  WiFi.macAddress(mac);
  if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0) {
    Logger::warn("MAC address is empty; hostname may not be unique");
  }
  std::snprintf(hostname_, sizeof(hostname_), "%s%02X%02X%02X",
                HOSTNAME_PREFIX,
                mac[3],
                mac[4],
                mac[5]);
}

bool NetworkManager::credentialsAccepted(const char* ssid, const char* password) const {
  if (ssid == nullptr || ssid[0] == '\0' || std::strcmp(ssid, "YOUR_WIFI") == 0) {
    Logger::warn("WiFi SSID missing. Copy include/secrets.example.h to include/secrets.h");
    return false;
  }
  if (password != nullptr && std::strcmp(password, "YOUR_PASSWORD") == 0) {
    Logger::warn("WiFi password is still the example placeholder");
    return false;
  }
  const size_t passwordLength = password != nullptr ? std::strlen(password) : 0;
  if (std::strlen(ssid) > WIFI_SSID_MAX_LENGTH || passwordLength > WIFI_PASSWORD_MAX_LENGTH) {
    Logger::error("WiFi credentials are longer than the 802.11 limit");
    return false;
  }
  return true;
}
