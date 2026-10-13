# Desarrollo

## Entorno

Hace falta PlatformIO Core o la extensión PlatformIO IDE en VS Code. Python 3 es suficiente para el Core.

```bash
pip install --user platformio
pio run -e esp12f
```

Abre la carpeta del proyecto, no un subdirectorio. El entorno por defecto es `esp12f`.

La verificación de este árbol usó PlatformIO 6.2.0, `espressif8266` 4.2.1 y el core Arduino 3.1.2. La plataforma está fijada en `platformio.ini` para que el mapa de flash no cambie sin querer.

## Secretos

```bash
cp include/secrets.example.h include/secrets.h
```

Edita los tres `define`. El archivo real está en `.gitignore`. El proyecto compila sin él: verás el aviso del compilador y, en serie, `WiFi disabled, local control continues`.

## Compilación

```bash
pio run -e esp12f          # binario, sin programar
pio run -e esp12f -t upload
pio device monitor -e esp12f
```

Monitor a 115200. El filtro `esp8266_exception_decoder` decodifica un stack dump si el ELF de esa compilación sigue en `.pio/`.

OTA, solo cuando el equipo ya está en la red:

```bash
pio run -e esp12f_ota -t upload
```

Antes, `upload_port` y `--auth` en `platformio.ini`. Detalle en [OTA.md](OTA.md).

## Carga física

UART 3,3 V, GPIO0 bajo, pulso de RST, EN alto, GPIO15 bajo. Velocidad 115200. Procedimiento en [WIRING.md](WIRING.md).

Si `esptool` dice que no sincroniza, el módulo no está en modo programación o el TX/RX está cruzado al revés, o el adaptador saca 5 V y el chip ya no responde.

## Depuración

El log INFO es el camino normal. DEBUG enseña cada muestra y la recuperación del bus:

```cpp
Logger::setLevel(LogLevel::Debug);
```

No imprimas la contraseña WiFi ni la de OTA.

`I2C_SCAN_ON_BOOT` en `Config.h` lista direcciones. Déjalo en `false` en cuanto identifiques el LCD: el barrido puede confundir al AHT10.

Pruebas de host, sin placa:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Isrc -o /tmp/filament-dryer-host-tests tools/host_tests.cpp
/tmp/filament-dryer-host-tests
```

No hay banco hardware en el entorno donde se escribió V0.1. La compilación y esas pruebas están hechas. Falta, con la placa delante:

- ACK del AHT10 en `0x38` y del LCD en `0x27`.
- Una lectura creíble frente a otro higrómetro.
- LCD estable mientras el sensor se actualiza (el bus compartido es el riesgo).
- Arranque con WiFi mal a propósito: el LCD sigue.
- OTA con contraseña buena y con contraseña mala.
- Corte de alimentación durante una carga, sabiendo que puede exigir el UART.

## Dónde va una clase nueva

| Si la clase… | Vive en |
| --- | --- |
| Habla con un periférico | `src/hardware/` |
| Habla con la red o con el mantenimiento | `src/network/` o `src/ota/` |
| Decide el proceso de secado | `src/control/` |
| Es un dato sin comportamiento de hardware | `src/models/` |
| Es un intervalo o un pin | `src/config/` |

`main.cpp` solo construye y ordena las llamadas. Un actuador nuevo implementa `Actuator` y se pasa al controlador. No añadas `digitalWrite` del calefactor dentro de `DryerController`.

Los tiempos nuevos son `constexpr` en `Config.h`. El lazo no usa `delay()`. La resta de `millis()` es la de `timeElapsed()`.

No crees una clase llamada `WiFiManager`. Ese nombre ya es una librería.

## Convenciones

- C++ del core Arduino, sin excepciones de aplicación y sin `new`.
- Nombres en inglés en el código. La documentación de este repositorio está en español.
- Un módulo no incluye `ArduinoOTA.h` ni `ESP8266WiFi.h` si no es su dueño.
- Lectura inválida: `valid == false` y `NAN`. Nunca `0.0` como centinela.
- Comentarios solo donde la hoja de datos, el boot o la seguridad no se leen solos.
- Warnings `-Wall -Wextra`. El aviso de `secrets.h` ausente es intencionado.

## Versión

Sube `FIRMWARE_VERSION` en `Config.h` al publicar un cambio (`0.1.1` para una corrección, `0.2.0` cuando entre el control térmico). El log de arranque es la comprobación de que la imagen nueva es la que corre.
