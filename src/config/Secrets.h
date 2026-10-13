#pragma once

// Unica inclusion de credenciales. main.cpp es el unico consumidor:
// el resto del firmware recibe punteros y no vuelve a leer este archivo.

#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#    define FILAMENT_DRYER_HAS_SECRETS 1
#  endif
#endif

#ifndef FILAMENT_DRYER_HAS_SECRETS
#  define FILAMENT_DRYER_HAS_SECRETS 0
#  warning "include/secrets.h no existe. Copia include/secrets.example.h. WiFi y OTA quedan desactivados."
#endif

#ifndef WIFI_SSID
#  define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#  define WIFI_PASSWORD ""
#endif

#ifndef OTA_PASSWORD
#  define OTA_PASSWORD ""
#endif
