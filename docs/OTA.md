# OTA

La placa de producto no lleva USB. La primera imagen entra por el UART. Las siguientes, por ArduinoOTA dentro de la LAN, usando PlatformIO y el protocolo `espota`.

No hay página web de actualización, ni ElegantOTA, ni un servidor HTTP propio. Eso queda fuera de V0.1.

```text
Programación física inicial
        |
        v
ESP-12F ejecutando V0.1 con el layout OTA
        |
        v
Asociación WiFi y OTA READY
        |
        v
pio run -e esp12f_ota -t upload
        |
        v
Reinicio y nueva versión
```

## Primera carga

Sigue el cableado de [WIRING.md](WIRING.md). Resumen:

1. UART 3,3 V. TX del adaptador al RX del módulo (GPIO3), RX del adaptador al TX del módulo (GPIO1), GND común.
2. EN alto, GPIO15 bajo, GPIO2 no forzado a masa.
3. GPIO0 bajo y pulso en RST.
4. Carga:

```bash
pio run -e esp12f -t upload
```

5. GPIO0 alto y reset. Abre el monitor:

```bash
pio device monitor -e esp12f
```

Tienes que ver `Firmware version: 0.1.0`. Si el WiFi asoció, también `IP:` y `OTA READY`.

La primera imagen ya usa `eagle.flash.4m2m.ld`. Si se grabó con otro mapa, OTA no tiene hueco hasta repetir la carga serie con este `platformio.ini`.

Para fábrica o servicio bastan pads de 3V3, GND, TX, RX, GPIO0, RST y EN. El usuario final no actualiza por ahí.

## Credenciales

```bash
cp include/secrets.example.h include/secrets.h
```

`OTA_PASSWORD` del header y `--auth=` de `platformio.ini` tienen que ser la misma cadena. El firmware llama a `ArduinoOTA.setPassword()` con ese valor. No está escrito en los archivos versionados.

En el entorno `esp12f_ota`:

```ini
upload_protocol = espota
upload_port = FILAMENT-DRYER-XXXXXX.local
upload_flags =
    --port=8266
    --auth=CHANGE_ME
```

Sustituye el hostname y `CHANGE_ME`. Esas dos líneas son configuración local. Si el repositorio es compartido, no dejes la contraseña real en un commit.

`upload_port` también acepta la IP, que es más fiable cuando mDNS no resuelve:

```ini
upload_port = 192.168.1.20
```

Los flags son los de `espota.py` del core: `--port` (por defecto 8266), `--auth` y, si algún día hubiera un sistema de archivos, `--spiffs`. V0.1 no sube un filesystem.

## Compilación y carga OTA

```bash
pio run -e esp12f
pio run -e esp12f_ota -t upload
```

El entorno por defecto sigue siendo `esp12f`. El botón Upload del IDE, si apunta al entorno por defecto, programa por cable. Hay que elegir `esp12f_ota` a propósito.

Tras un OTA correcto el módulo reinicia solo (`setRebootOnSuccess(true)`). El log de la imagen nueva debe mostrar la versión y `Software/System restart`. Los actuadores futuros tienen que seguir apagados: `forceSafeActuatorState()` es lo primero que hace el arranque, y en V0.1 no hay ninguno.

## Identificación

El hostname es `FILAMENT-DRYER-` más los tres últimos bytes de la MAC:

```text
FILAMENT-DRYER-A1B2C3
```

mDNS anuncia `FILAMENT-DRYER-A1B2C3.local` cuando `ArduinoOTA.begin()` arranca con mDNS, que es el caso. El nombre es insensible a mayúsculas en mDNS. Hace falta un resolutor (Avahi en Linux, Bonjour en Windows). En la misma red puedes ver el servicio `_arduino._tcp`.

Varios secadores conviven porque cada MAC produce un sufijo distinto. Si el log dice `MAC address is empty`, el sufijo `000000` no es único y hay un fallo de radio o de arranque.

## Seguridad

- La contraseña es obligatoria. Sin ella, `OTAManager` no llama a `ArduinoOTA.begin()`.
- El reto es Digest-MD5 entre `espota.py` y el core. La imagen en sí viaja en claro por la LAN. Quien esté en la red y no tenga la clave no debe poder escribir la flash; quien esté en la red y capture el tráfico puede ver el binario.
- El puerto es UDP 8266, solo en la red local. No reenvíes ese puerto en el router. No hay cifrado de transporte ni autenticación de más de una clave.
- Una contraseña corta se adivina. Usa una larga y distinta de la del WiFi.
- Perder la clave obliga a una carga serie.

## Qué pasa cuando algo falla

Esto es lo que hace el core Arduino 3.1.2 con `eboot`, no una promesa añadida.

1. PlatformIO envía el binario por UDP al hueco libre que hay entre el sketch actual y el sistema de archivos.
2. El `Updater` comprueba el MD5.
3. Si la transferencia se corta (WiFi, cliente, reset) antes de cerrar con éxito, el comando de arranque no cambia y **sigue ejecutándose el firmware anterior**.
4. Si el MD5 cuadra, el core guarda la orden de copia en los **primeros 128 bytes de la RTC del usuario** y reinicia. Esos 128 bytes no deben usarse para datos propios: OTA los pisa. V0.1 no usa la RTC.
5. `eboot` copia la imagen nueva **encima** de la vieja y salta a ella.

No hay particiones A/B con comprobación de salud. No hay rollback automático.

| Suceso | Resultado real |
| --- | --- |
| WiFi cae durante la transferencia | Error OTA (`receive` o `connect`). El firmware anterior sigue, salvo que la copia de `eboot` ya hubiera empezado |
| Corte de alimentación durante la recepción | Normalmente arranca la imagen vieja, porque el flag aún no se escribió |
| Corte de alimentación mientras `eboot` copia | Puede quedar un módulo que no arranca. La recuperación es el UART |
| Imagen que arranca y entra en bucle de excepción o watchdog | No vuelve sola a la versión previa. Hace falta el programador |
| Contraseña incorrecta | `OTA ERROR: auth`. No se escribe la flash |
| Firmware demasiado grande | La carga ni empieza: PlatformIO rechaza pasar de 1044464 bytes |
| WiFi apagado | OTA no está disponible. Sensor y LCD siguen |

El log `OTA END` sale justo antes del reinicio. Si no vuelve, no asumas que la copia terminó bien: conecta el UART.

## OTA y el calefactor que todavía no existe

Cuando haya resistencia, la regla será no aceptar OTA con el elemento activo. El gancho ya está: `DryerController::isMaintenanceAllowed()` devuelve false si un `Actuator` informa que está habilitado. En V0.1 el puntero es nulo y la función devuelve true.

Eso no es la secuencia completa. La que habrá que implementar antes de conectar una resistencia es:

```text
Secado activo
    -> petición OTA
    -> detener el proceso
    -> calefactor OFF
    -> ventilación o estado seguro
    -> solo entonces OTA
    -> reinicio
    -> calefactor OFF otra vez
    -> validar sensores
    -> listo, sin reanudar el calor por una bandera guardada
```

`ArduinoOTA.onStart` llega tarde para "pedir permiso": la sesión ya empezó. La decisión tiene que tomarse antes, no llamando a `handle()` cuando el estado no es seguro, y `onStart` solo puede forzar `forceOff()` como último recurso. V0.1 no tiene nada que apagar, así que `onStart` únicamente registra `OTA START`.

## Síntomas

| Síntoma | Qué mirar |
| --- | --- |
| El dispositivo no aparece | ¿Hay `OTA READY` en serie? ¿Misma LAN, sin red de invitados ni aislamiento de clientes? |
| El hostname no resuelve | Usa la IP. Comprueba mDNS en el PC |
| IP incorrecta | El log `IP:` es el que vale. Un lease viejo no |
| Contraseña rechazada | `OTA_PASSWORD` y `--auth` tienen que coincidir. El log dirá `OTA ERROR: auth` |
| WiFi desconectado | No hay OTA. El LCD debe seguir mostrando la medida o `AHT10 ERROR` |
| OTA no disponible | Falta `secrets.h`, la clave sigue siendo el ejemplo, o el intento WiFi agotó el timeout |
| Se fue la luz a mitad | Si no arranca, carga serie. No hay un segundo banco que rescate solo |
