# Firmware

Versión de referencia: **0.1.0**, definida solo en `src/config/Config.h` (`FIRMWARE_VERSION`). El resto del árbol no duplica esa cadena.

## Toolchain

| Pieza | Versión usada al verificar este árbol |
| --- | --- |
| PlatformIO | 6.2.0, plataforma `espressif8266@4.2.1` |
| Core Arduino ESP8266 | 3.1.2 (`framework-arduinoespressif8266` 3.30102.0) |
| Placa PlatformIO | `esp12e` (el módulo real es un ESP-12F) |
| Script de enlace | `eagle.flash.4m2m.ld` |
| Librería LCD | `marcoschwartz/LiquidCrystal_I2C` 1.1.4 |
| Framework | Arduino. No FreeRTOS |

```bash
pio run -e esp12f
```

Resultado de esa compilación: flash **328964 / 1044464** bytes (31,5 %), RAM **31440 / 81920** bytes (38,4 %). El techo de 1044464 bytes es el hueco del sketch con OTA. Superarlo rompe la carga.

## Configuración

`include/secrets.h` (no versionado):

```cpp
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define OTA_PASSWORD "YOUR_OTA_PASSWORD"
```

Si el archivo no existe, el compilador emite un aviso y el firmware arranca sin radio. Las cadenas de ejemplo `YOUR_WIFI`, `YOUR_PASSWORD` y `YOUR_OTA_PASSWORD` se rechazan en ejecución: no intenta asociarse a una red llamada `YOUR_WIFI`.

Una red abierta puede usar contraseña vacía. La contraseña OTA no puede estar vacía: sin ella el servicio OTA no arranca.

Intervalos y pines están en `src/config/Config.h` y `src/config/Pins.h`. No dejes números sueltos en los `.cpp`.

| Constante | Valor | Uso |
| --- | --- | --- |
| `SENSOR_INTERVAL_MS` | 2000 | Periodo entre medidas, contado al terminar la anterior |
| `SENSOR_MEASURE_DELAY_MS` | 80 | Espera mínima tras `0xAC` (la hoja pide más de 75 ms) |
| `SENSOR_MEASURE_TIMEOUT_MS` | 200 | Si sigue ocupado, la muestra es un timeout |
| `SENSOR_RETRY_INTERVAL_MS` | 5000 | Reintento tras un fallo de comunicación |
| `SENSOR_MAX_FAILURES` | 3 | Fallos seguidos antes de pasar a `SensorError` si había una lectura buena |
| `DISPLAY_INTERVAL_MS` | 1000 | Cadencia mínima de repintado si nada cambió |
| `DISPLAY_REFRESH_MS` | 10000 | Reescribe las dos líneas aunque el texto coincida, por si el LCD se corrompió |
| `WIFI_CONNECT_TIMEOUT_MS` | 15000 | Tope de un intento de asociación |
| `WIFI_RETRY_INTERVAL_MS` | 10000 | Espera antes de volver a llamar a `WiFi.begin()` |
| Reloj I2C | el de `Wire.begin()` | 100 kHz y 150 ms de stretch, los valores del core. No se llama a `setClock()` |
| `I2C_SCAN_ON_BOOT` | false | Un barrido `0x03`–`0x77` molesta al AHT10. Solo para diagnóstico |

## Librerías

| Librería | Papel |
| --- | --- |
| Wire, ESP8266WiFi, ArduinoOTA | Core. No se reimplementan |
| LiquidCrystal_I2C | HD44780 a través del PCF8574. La misma secuencia `init()` y `backlight()` que ya funcionó en este ESP-12F |

No hay librería de terceros para el AHT10. El motivo está en [ARCHITECTURE.md](ARCHITECTURE.md): las implementaciones habituales bloquean durante la conversión.

## Ciclo

`setup()` registra la versión, el motivo de reset y el estado seguro, inicia I2C, el LCD y el sensor, y lanza el intento WiFi. No espera a ninguno.

`loop()` actualiza red y OTA. Si no hay una transferencia en curso, avanza un paso del sensor, el controlador y la pantalla. Cada `update()` del sensor hace como mucho una transacción I2C.

## Temporización del AHT10

```text
StartupWait 40 ms
    -> soft reset 0xBA, hasta 3 intentos
    -> espera 20 ms
    -> init 0xE1 0x08 0x00
    -> espera 10 ms
    -> medida 0xAC 0x33 0x00
    -> espera 80 ms y lee 6 bytes
    -> si el bit Busy sigue a 1, relee cada 20 ms hasta 200 ms
    -> reposo hasta completar 2000 ms
```

Comandos según la hoja ASAIR: inicialización `0xE1`, disparo `0xAC`, reset `0xBA`. Los bytes `0x08 0x00` y `0x33 0x00` son la secuencia de parámetros que usa el propio diagrama de disparo y las implementaciones contrastadas con el circuito. El bit 7 del estado es Busy. El bit 3 es la calibración.

Conversión:

```text
HR% = raw20 / 1048576 * 100
T°C = raw20 / 1048576 * 200 - 50
```

Fuera de −40…85 °C o de 0…100 % HR la muestra es `OutOfRange` y no se enseña. Un 20 bits no puede codificar más del 100 % de humedad; el rechazo de temperatura sí cubre el raw a cero (−50 °C) y el raw a tope (~150 °C), que son patrones típicos de un bus en corto o flotando.

## Estados del controlador

| Estado | Cuándo | LCD |
| --- | --- | --- |
| `Initializing` | El sensor aún no cerró el primer intento | `FILAMENT DRYER` / `Starting...` |
| `Idle` | Hay una muestra válida, o menos de 3 fallos tras una válida | `Temp: 25.4 C` / `Hum : 48.2 %` |
| `SensorError` | No hubo muestra válida, o se agotaron los 3 fallos | `AHT10 ERROR` y una segunda línea |

Segundas líneas de error: `Check sensor`, `I2C timeout`, `I2C error`, `Out of range`, `Not calibrated`.

El LCD solo reescribe una línea si sus 16 caracteres cambiaron. No llama a `clear()` en el lazo, para no parpadear.

## WiFi

Orden: `persistent(false)`, modo STA, MAC, hostname, y solo entonces `WiFi.begin()` si las credenciales sirven. El hostname tiene que fijarse antes de `begin()`; si no, el stack lo ignora.

Formato: `FILAMENT-DRYER-` más los tres últimos octetos de la MAC en hex mayúsculas. Ejemplo: `FILAMENT-DRYER-A1B2C3`. Cabe en el límite de 32 caracteres del ESP8266.

`WIFI_NONE_SLEEP` mantiene la radio despierta. En un aparato de red eléctrica importa más que OTA no pierda paquetes que ahorrar miliamperios.

El lazo solo consulta `WiFi.status()`. El reintento llama a `WiFi.begin()` cada 10 s después de un timeout, no en cada vuelta.

## Logging

Serie a 115200 8N1. Niveles `[DEBUG]`, `[INFO]`, `[WARN]`, `[ERROR]`. El nivel por defecto es INFO. Para ver cada muestra:

```cpp
Logger::setLevel(LogLevel::Debug);
```

Mensajes que deben aparecer:

```text
[INFO] Firmware version: 0.1.0
[INFO] Reset reason: Power On
[INFO] Safe state applied: no power actuators in V0.1
[INFO] I2C SDA=GPIO4 SCL=GPIO5
[INFO] LCD detected at 0x27
[INFO] Hostname: FILAMENT-DRYER-A1B2C3
[INFO] AHT10 detected at 0x38
[INFO] IP: 192.168.1.20
[INFO] OTA READY
```

OTA, cuando ocurre:

```text
[INFO] OTA START
[INFO] OTA progress: 25%
[INFO] OTA END
[ERROR] OTA ERROR: receive
```

El progreso solo se anota en 25, 50 y 75 %. Las lecturas periódicas van a DEBUG. Un sensor desconectado avisa una vez en WARN y los reintentos bajan a DEBUG hasta que vuelve.

Motivos de reset, los mismos textos que `ESP.getResetReason()` del core 3.1.2:

| Código | Texto | ¿Inesperado? |
| --- | --- | --- |
| `REASON_DEFAULT_RST` | Power On | No |
| `REASON_EXT_SYS_RST` | External System | No |
| `REASON_SOFT_RESTART` | Software/System restart | No. También es el reinicio de un OTA correcto. V0.1 no distingue ambos |
| `REASON_DEEP_SLEEP_AWAKE` | Deep-Sleep Wake | No. Este firmware no entra en deep sleep |
| `REASON_WDT_RST` | Hardware Watchdog | Sí |
| `REASON_SOFT_WDT_RST` | Software Watchdog | Sí |
| `REASON_EXCEPTION_RST` | Exception, con `cause` y `epc1` | Sí |

## Watchdog

Hay dos, los del SDK NONOS, activos por el core Arduino:

- El de software reinicia el chip si el código no cede la CPU. Medidas públicas sobre este core lo sitúan en torno a **3,2 s** sin `yield()` ni retorno de `loop()`. `ESP.wdtEnable(timeout)` **ignora** el argumento: el core lo documenta así y no sirve para elegir otro tiempo.
- El de hardware actúa si el de software se deja parado demasiado tiempo. El comentario del core pide no tenerlo parado más de unos 6 s; medidas de la comunidad lo sitúan cerca de 8 s.

`delay()` y `yield()` alimentan el watchdog. `delayMicroseconds()` no. Este firmware no llama a `wdtDisable()`, `wdtEnable()` ni `wdtFeed()`. La recuperación ante un cuelgue es diseñar el `loop` para que vuelva, no alimentar el perro desde un bucle bloqueante.

Un reinicio inesperado se ve en el primer log, como `Unexpected reset: ...`. No hay contador en flash.

## OTA dentro del firmware

`OTAManager` es el único archivo que incluye `ArduinoOTA.h`. `begin()` configura hostname, puerto 8266, contraseña y reinicio al terminar, y registra los callbacks. `update()` llama a `ArduinoOTA.handle()`.

`ArduinoOTA.begin()` no hace nada si el servicio ya estaba iniciado. Por eso, al caer el WiFi, `suspend()` llama a `ArduinoOTA.end()` (API real del core 3.1.2) y la siguiente asociación puede volver a abrirlo. No se llama a `end()` en medio de una transferencia.

La contraseña viaja al método `setPassword()`, que el core guarda como hash MD5 para el reto de `espota`. No se imprime.

## Comprobaciones de host

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Isrc -o /tmp/filament-dryer-host-tests tools/host_tests.cpp
/tmp/filament-dryer-host-tests
```

Cubren el punto 25 °C / 50 % HR, el punto 50 °C / 50 % HR, Busy, no calibrado, fuera de rango y el formateo a un decimal. No sustituyen una prueba con el sensor conectado.

## Qué no está en el binario

No hay PID, perfiles, servidor web, HTTP OTA, almacenamiento de configuración, ni salida de potencia. El contrato `Actuator` compila como interfaz y no tiene instancia.
