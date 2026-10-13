#pragma once

#include <stdint.h>

#include <IPAddress.h>

// Conectividad WiFi en segundo plano. No se llama WiFiManager para no
// confundirla con la libreria externa de ese nombre.
// El secador local no espera a esta clase para medir ni para mostrar datos.
class NetworkManager {
 public:
  bool begin(const char* ssid, const char* password);
  void update();
  void reconnect();

  bool isConnected() const { return state_ == LinkState::Connected; }
  bool isDisabled() const { return state_ == LinkState::Disabled; }
  bool hasConnectFailed() const { return state_ == LinkState::WaitingRetry; }
  IPAddress getIP() const;
  const char* hostname() const { return hostname_; }

 private:
  enum class LinkState : uint8_t {
    Disabled,
    Connecting,
    Connected,
    WaitingRetry
  };

  void buildHostname();
  bool credentialsAccepted(const char* ssid, const char* password) const;

  const char* ssid_ = "";
  const char* password_ = "";
  char hostname_[32] = {};
  LinkState state_ = LinkState::Disabled;
  unsigned long attemptStartedMs_ = 0;
  unsigned long retryStartedMs_ = 0;
};
