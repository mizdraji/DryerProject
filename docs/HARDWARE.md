# Hardware

V0.1 observa temperatura y humedad. No conmuta red eléctrica.

## ESP-12F

| Dato | Valor |
| --- | --- |
| SoC | ESP8266EX |
| Módulo | ESP-12F (AI-Thinker y equivalentes) |
| Flash típica | 4 MB (32 Mbit), SPI, modo DIO |
| CPU en este firmware | 80 MHz |
| Reloj de flash | 40 MHz |
| Alimentación | 3,0–3,6 V, nominal **3,3 V** |
| GPIO | No toleran 5 V. Máximo absoluto habitual 3,6 V |
| ADC `TOUT` | 0–1,0 V. No es una entrada de 3,3 V |

PlatformIO no define una placa `esp12f`. El id `esp12e` describe el mismo ESP8266EX con 4 MB y es el que usa este proyecto. Confirma la serigrafía del módulo: si la flash fuera de 1 MB, este mapa OTA no cabe.

El pico de la radio puede superar 300 mA. La fuente de 3,3 V tiene que poder entregar **al menos 500 mA** de forma continua, con el condensador de desacoplo junto al módulo (100 nF y otro electrolítico de 10–100 µF). No alimentes el módulo desde el pin de 3,3 V de un adaptador USB-UART barato: se hunde al asociar el WiFi y el equipo reinicia.

Pines de arranque. Si no se respetan, el módulo no sale del boot o entra siempre en programación.

| GPIO | Exigencia en reset | Uso en V0.1 |
| --- | --- | --- |
| GPIO0 | Alto para arrancar. Bajo para grabar por UART | Solo programación. Pull-up 10 kΩ a 3,3 V |
| GPIO2 | Alto (o flotante con el pull-up interno). No puede estar a masa | Sin uso. No poner un LED a masa que lo fuerce a bajo |
| GPIO15 | Bajo | Pull-down 10 kΩ a GND. Sin uso de aplicación |
| GPIO1 / TX | UART | Log y programación |
| GPIO3 / RX | UART | Log y programación |
| GPIO4 | Libre | SDA |
| GPIO5 | Libre | SCL |
| GPIO6–GPIO11 | Conectados a la flash interna | Prohibidos |
| GPIO16 | Sin interrupción. A menudo unido a RST para deep sleep | Sin uso |
| EN / CH_PD | Alto para ejecutar | Pull-up 10 kΩ a 3,3 V. No dejarlo flotante |
| RST | Alto para ejecutar | Pull-up 10 kΩ y pulsador a GND |

GPIO4 y GPIO5 no participan en el muestreo de boot. Por eso son el par I2C. En una placa NodeMCU esos pines se llaman **D2 (GPIO4, SDA)** y **D1 (GPIO5, SCL)**. El módulo desnudo no trae esa serigrafía.

## AHT10

Hoja ASAIR (borrador en inglés usado como referencia):

| Dato | Valor |
| --- | --- |
| Dirección I2C | `0x38` (7 bit), fija |
| VDD | 1,8–3,6 V. Recomendado 3,3 V |
| Máximo en SDA/SCL | VDD + 0,3 V |
| Tras el encendido | Hasta 20 ms hasta aceptar comandos |
| Medida | Comando `0xAC`, esperar más de 75 ms |
| Rango | −40…85 °C, 0…100 % HR |
| Desacoplo | 100 nF entre VDD y GND, pegado al sensor |
| Pull-up de ejemplo | 10 kΩ hacia VDD |

La misma nota de aplicación dice tres cosas que este proyecto no puede ignorar:

1. La tensión del maestro debe coincidir con la del sensor.
2. Solo un AHT10 en el bus, y ningún otro dispositivo I2C.
3. El maestro solo debe poner SDA y SCL a nivel bajo. El nivel alto lo dan los pull-up.

V0.1 comparte el bus con el LCD porque ese es el esquema pedido. Es una desviación de la hoja de datos. En la práctica a veces funciona si los cables son cortos, el reloj es de 100 kHz y no se barre el bus. También falla: el AHT10 puede interpretar tráfico ajeno. El firmware no lo "arregla". Si al actualizar el LCD se pierden medidas, la solución conforme es separar los dispositivos (multiplexor I2C, por ejemplo TCA9548A, con el AHT10 solo en un canal) o cambiar a un sensor especificado para bus compartido, como el AHT20. Ninguna de las dos está implementada.

El AHT10 no sirve como único límite de seguridad térmica. Deja de estar en especificación a 85 °C, y un secador en fallo puede superar eso. Cuando exista resistencia, la protección tiene que ser independiente del firmware y de este sensor. Ver [SAFETY.md](SAFETY.md).

## LCD 16x2 I2C

Dirección configurada: **`0x27`** (PCF8574). Un PCF8574A suele responder en `0x3F`. Si `0x27` no contesta, el arranque prueba el resto del mapa del backpack (sin tocar `0x38`, que es el AHT10) y deja la dirección en el log. Hay que copiarla a `LCD_I2C_ADDRESS` en `src/config/Pins.h`.

La librería `LiquidCrystal_I2C` asume el cableado de backpack más habitual:

| Bit del PCF8574 | Señal |
| --- | --- |
| P0 | RS |
| P1 | RW |
| P2 | E |
| P3 | Backlight |
| P4–P7 | D4–D7 |

Esta librería no permite reasignar esos bits. Un backpack con otro mapa no va a mostrar caracteres.

### El backpack a 5 V no es compatible directo

La mayoría de estos módulos se diseñaron para 5 V:

- El PCF8574 y el HD44780 se alimentan a 5 V.
- Hay pull-up de unos 4,7 kΩ de SDA y SCL hacia esos 5 V.
- Con el PCF8574 a 5 V, el nivel alto mínimo de entrada es aproximadamente `0,7 × VCC` (3,5 V). Un "1" de 3,3 V del ESP8266 puede no reconocerlo.
- Esos 5 V superan el máximo de los GPIO del ESP8266 y el máximo `VDD + 0,3 V` del AHT10.

Conectarlos directamente puede dañar el módulo o el sensor y hace la comunicación inestable. No se asume que "cualquier LCD I2C funciona a 3,3 V".

### Opciones eléctricas

**Opción A, un solo bus a 3,3 V.** Alimentar el backpack desde 3,3 V, de forma que sus pull-up suban a 3,3 V. El PCF8574 admite desde unos 2,5 V. Muchos HD44780 arrancan a 3,3 V, pero el contraste se queda corto: hay que ajustar el potenciómetro. Si no aparecen caracteres, ese panel necesita 5 V y no vale esta opción. El AHT10 permanece a 3,3 V. El bus sigue compartido; la nota del fabricante no desaparece.

**Opción B, desplazador de nivel bidireccional.** Es la opción correcta si el LCD va a 5 V.

```text
3,3 V                                5 V
ESP8266 GPIO4/5 ----+---- AHT10      LCD backpack
                    |
               LV del shifter ---- HV del shifter
               pull-up a 3,3 V       pull-up a 5 V (los del backpack)
```

Sirve un módulo MOSFET (BSS138) o un PCA9306. Tierra común. El AHT10 se queda **solo** en el lado de 3,3 V. El shifter no aísla el protocolo: el AHT10 sigue viendo el tráfico del LCD. Resuelve la tensión, no la nota de "un solo dispositivo".

**No usar** un divisor resistivo en SDA/SCL. El bus es bidireccional.

### Pull-up

Un solo grupo de pull-up por lado del bus, hacia la alimentación de ese lado. Valor típico 4,7 kΩ o 10 kΩ. Los pull-up internos del ESP8266 son débiles y no bastan. El firmware los activa como ayuda, no como sustituto.

Si el módulo AHT10 ya trae 10 kΩ a su VDD y el VDD es 3,3 V, no añadas otro juego fuerte en paralelo sin medir el flanco. Dos resistencias de 4,7 kΩ en paralelo siguen siendo usables a 100 kHz, pero conviene saber cuántas hay.

Reloj del bus: el que deja `Wire.begin()` en este core, **100 kHz** y 150 ms de clock stretch. No se llama a `setClock()` ni se acorta el stretch: con un límite de 10 ms este LCD y el AHT10 respondían NACK.

## Mapa de flash y OTA

Script de enlace: `eagle.flash.4m2m.ld` (Arduino ESP8266 3.1.2). Comentarios del propio script, direcciones en el mapa de memoria del núcleo (la flash se ve en `0x40200000`):

| Región | Dirección mapeada | Tamaño |
| --- | --- | --- |
| Sketch | `0x40200000` | 1044464 bytes (~1019 KB) útiles para la aplicación |
| Hueco OTA | `0x402FEFF0` | ~1028 KB. Ahí se escribe la imagen nueva |
| Sistema de archivos | `0x40400000` | ~2024 KB. V0.1 no lo monta |
| EEPROM emulada | `0x405FB000` | 4 KB. V0.1 no la usa |
| Calibración RF | `0x405FC000` | 4 KB. La toca el SDK |
| Configuración WiFi del SDK | `0x405FD000` | 12 KB. La toca el SDK |

En offsets del chip de 4 MB (dirección mapeada menos `0x40200000`): el sistema de archivos empieza en `0x200000`, la EEPROM en `0x3FB000`, la calibración RF en `0x3FC000` y la config WiFi en `0x3FD000`.

El hueco entre el sketch y el sistema de archivos es el que usa ArduinoOTA para depositar el binario nuevo. Por eso el sketch no puede pasar de **1044464 bytes**. Un layout "4 MB sin reservar el segundo megabyte" no da OTA. El layout `eagle.flash.4m3m.ld` (FS de 3 MB) baja el techo OTA a unos 512 KB. No se usa.

La primera grabación ya tiene que usar este script. Cambiarlo después exige una carga serie.

Medida de referencia de V0.1.0: **328364 / 1044464 bytes** de flash y **31168 / 81920 bytes** de RAM. Queda sitio para crecer, no sitio infinito.

## Expansión prevista, no construida

| Función | Notas de hardware |
| --- | --- |
| Resistencia | SSR o MOSFET según la carga. El GPIO de mando debe quedar en OFF con el pin en alta impedancia durante el boot. Pull externo hacia el estado apagado. Candidatos de GPIO: 12, 13 o 14. Prohibidos GPIO0, GPIO2, GPIO15 y GPIO6–11 |
| Ventilador | MOSFET o transistor en baja tensión, mismo criterio de reposo en OFF |
| Encoder y botones | Entradas con pull-up a 3,3 V. No usar pines de boot |
| Buzzer | GPIO de salida, silencio en reset |
| Segundo sensor o tapa | Otro dispositivo en este I2C choca con la limitación del AHT10. Mejor otro bus, un multiplexor, o 1-Wire si el sensor lo permite |
| NTC en el ADC | El ADC del ESP8266 es de 0–1 V, no 0–3,3 V. Hace falta divisor y no sustituye un termostato de seguridad |
| Protección | Fusible, termostato o fusible térmico en serie con la resistencia, independientes del firmware |

Ningún pin de potencia está definido en el código a propósito: un `#define` sin usar invita a cablearlo antes de tener el estado seguro.
