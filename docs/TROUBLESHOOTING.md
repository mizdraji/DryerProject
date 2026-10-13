# Problemas frecuentes

| Problema | Posible causa | Qué hacer |
| --- | --- | --- |
| AHT10 no detectado | SDA/SCL cruzados, sin masa común, sensor a 5 V, pull-up a 5 V | Comprobar GPIO4 = SDA, GPIO5 = SCL, VDD = 3,3 V. El log debe decir `AHT10 error: not-connected` y el LCD `AHT10 ERROR` / `Check sensor` |
| Lecturas a rachas, LCD raro | El AHT10 no está especificado para compartir el bus | Acortar cables, dejar `I2C_SCAN_ON_BOOT` en false, bajar a 100 kHz (ya es el valor). Si persiste, multiplexor o sensor de bus compartido. Ver [HARDWARE.md](HARDWARE.md) |
| LCD en blanco con backlight | Contraste, o panel que no arranca a 3,3 V | Ajustar el potenciómetro. Si hace falta 5 V, poner shifter; no unir SDA a 5 V |
| LCD no responde | Dirección distinta de `0x27`, o el sondeo de ACK falló antes de `init()` | El panel se inicializa igual que el sketch de prueba. Si el ACK falla, el log trae `wire=2` (NACK) o `wire=4` (bus ocupado) y aun así escribe `FILAMENT DRYER`. El barrido completo (`I2C_SCAN_ON_BOOT`) sigue siendo solo para un diagnóstico puntual |
| Caracteres basura | Mapa de bits del PCF8574 distinto | `LiquidCrystal_I2C` fija P0=RS, P1=RW, P2=E, P3=luz, P4–P7=datos. Otro mapa no se puede reasignar con esta librería |
| El ESP no arranca | GPIO0 quedó a masa, EN flotante, GPIO15 alto, 3,3 V débil | Pull-up de GPIO0 y EN, pull-down de GPIO15, fuente de 500 mA |
| Reinicios al conectar el WiFi | Alimentación insuficiente | No usar el 3,3 V del adaptador USB. Mirar el log: `Hardware Watchdog` o `Software Watchdog` |
| `esptool` no sincroniza | No está en modo programación, TX/RX invertidos, adaptador a 5 V | GPIO0 bajo, pulso RST, cruzar TX/RX, medir la tensión del TX |
| WiFi no conecta | `secrets.h` ausente, SSID o clave de ejemplo, 2,4 GHz mal | El aviso del compilador y el log `WiFi SSID missing` o `WiFi timeout`. El LCD no debe congelarse |
| Hostname `...-000000` | MAC leída a cero | La radio no llegó a inicializarse. Revisar alimentación y el log anterior |
| OTA no aparece | Sin `OTA READY`, otra VLAN, aislamiento de clientes, mDNS | Misma red. Probar la IP del log en `upload_port` |
| Hostname no resuelve | El PC no tiene mDNS | `upload_port` con la IP |
| OTA rechaza la clave | `--auth` distinto de `OTA_PASSWORD` | Log `OTA ERROR: auth`. Corregir y no hace falta regrabar por cable si la clave del equipo es la buena |
| OTA no disponible | Clave vacía o `YOUR_OTA_PASSWORD`, o WiFi aún no asoció | Log `OTA disabled` o `OTA not available`. El lazo local sigue |
| La carga OTA supera el tamaño | Sketch por encima de 1044464 bytes | `pio run` ya lo rechaza. No cambies el ldscript para "ganar sitio" sin rehacer el cálculo del hueco |
| Tras OTA no arranca | Corte durante la copia de `eboot`, o imagen que falla al boot | No hay rollback. Recuperar por UART. Ver [OTA.md](OTA.md) |
| `Unexpected reset: Exception` | Bug, puntero, pila | El log trae `cause` y `epc1`. Monitor con el decoder de excepciones y el ELF de esa misma compilación |
| Temperatura `Out of range` | Bus colgado (lecturas extremas) o sensor fuera de −40…85 °C | `AHT10 out of range` en DEBUG. No uses esa cifra. Revisa pull-up y longitud |
| El equipo "mide" 0 °C y 0 % | Puede ser una medida real, o un raw que el decoder rechaza | 0 °C con humedad creíble y `valid` es una medida. Raw cero de temperatura (−50 °C) se descarta como fuera de rango y no se pinta como 0.0 |

El funcionamiento local no se diagnostica por la red. Si el WiFi falla y el LCD también, el problema es alimentación, I2C o el propio módulo, no la clave de la WiFi.
