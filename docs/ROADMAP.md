# Roadmap

Solo V0.1 está en este repositorio. El resto es el orden previsto, no código a medias.

```text
V0.1
├── AHT10
├── LCD 16x2
├── ESP-12F
├── WiFi con timeout
├── OTA por ArduinoOTA
└── Arquitectura del lazo local

V0.2
├── Resistencia, con estado seguro y corte térmico independiente
├── Ventilador
├── Control de temperatura
└── Protecciones (sensor perdido, tope, watchdog ya presente)

V0.3
├── Encoder
├── Botones
├── Menú en el LCD
└── Perfiles de secado

V0.4
├── Persistencia
├── Configuración en el equipo
└── Endurecer el arranque (nada guardado enciende el calor)

V0.5
├── Interfaz web
└── OTA por web

V1.0
└── Sistema completo, con el criterio de seguridad de SAFETY.md
```

## Condiciones antes de V0.2

No son funciones de V0.1. Son requisitos para no construir el control térmico encima de un bus o de una protección que no dan:

- Resolver la limitación del AHT10 en un bus compartido, con medidas reales LCD + sensor, o separar los dispositivos.
- Definir el pin de potencia, el estado eléctrico OFF durante el boot y el termostato independiente.
- Implementar la parada ordenada antes de un OTA, no solo el gancho `isMaintenanceAllowed()`.

## Fuera de alcance hasta que el roadmap lo pida

PID, perfiles, encoder, botones, buzzer, segundo sensor, EEPROM o LittleFS de configuración, servidor web, HTTP OTA, ElegantOTA, particiones A/B con rollback y portal cautivo de WiFi.
