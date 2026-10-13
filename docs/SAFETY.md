# Seguridad

V0.1 no calienta, no mueve aire y no conmuta red. Este documento fija las reglas para cuando eso exista. No describas esta versión como un secador térmico terminado.

El firmware no es un dispositivo de seguridad. Un fallo de software, de sensor o de alimentación no puede ser la última barrera delante de una resistencia.

## Estado seguro tras cualquier reset

```text
Después de un reset, un corte o un OTA:
actuadores peligrosos = OFF
```

En código, `forceSafeActuatorState()` se ejecuta al arrancar, antes de la red y antes del sensor. Hoy registra que no hay actuadores. Cuando exista uno, esa función tiene que llamar a `forceOff()` sin leer una preferencia guardada.

Prohibido: una variable en flash, EEPROM o RTC que, al arrancar, vuelva a encender la resistencia porque "el ciclo estaba a medias". Reanudar el calor exige una orden nueva, con sensores válidos y límites comprobados.

El GPIO de mando se elige para que el estado de alta impedancia del boot sea OFF (pull externo hacia apagado, lógica activa en el sentido que no dispare el SSR durante el glitch de reset). No uses GPIO0, GPIO2 ni GPIO15.

## Cuando exista calefactor

Aún no está implementado. El diseño previsto es:

```text
Consigna -> controlador -> actuador -> temperatura real -> sensor
```

El controlador hablará con la interfaz `Actuator`, no con `digitalWrite` de un pin concreto. No hay PID en V0.1.

Antes de la primera resistencia hacen falta, en hardware y fuera del ESP8266:

- Fusible en el circuito de potencia.
- Termostato o fusible térmico independiente, en serie con el elemento, que abra aunque el firmware esté colgado o el SSR en corto.
- Separación física y de aislamiento entre la lógica de 3,3 V y la red. El SSR o el relé tienen que estar dimensionados y con la línea de fuga que corresponda. La masa de lógica no se usa como retorno de red.
- El AHT10 deja de ser válido por encima de 85 °C. No es el sensor de esa protección.

Sobretemperatura, sensor desconectado o tapa abierta (cuando exista ese contacto) tienen que cortar el mando. Un sensor que desaparece del bus ya lleva a `SensorError` en V0.1; esa transición, en V0.2, será una de las condiciones para `forceOff()`, no un simple texto en el LCD.

## OTA

OTA es mantenimiento. Con la resistencia activa no debe aceptarse. La secuencia ordenada está en [OTA.md](OTA.md). V0.1 solo tiene el gancho `isMaintenanceAllowed()`, que hoy devuelve true porque no hay actuador.

Un OTA interrumpido en la fase de copia de `eboot` puede dejar el módulo sin arrancar. En un equipo con resistencia, un módulo muerto tiene que seguir siendo seguro: el elemento se queda OFF por hardware, no porque el programa lo diga.

No hay rollback. Una imagen mala no vuelve sola a la anterior.

El puerto UDP 8266 no sale a Internet. La clave OTA no se versiona. El binario no va cifrado dentro de la LAN.

## Alimentación

Un 3,3 V débil provoca reinicios al transmitir, flash a medias y lecturas absurdas. Eso, con una resistencia, es un fallo de seguridad además de uno de comunicación. Fuente holgada y desacoplo junto al módulo.

El watchdog de software (en torno a 3 s sin ceder la CPU) y el de hardware reinician el chip si el lazo se cuelga. Sirven para salir de un bloqueo, no para vigilar la temperatura. Al volver, el estado es OFF. No se usan para permitir código bloqueante.

## Pérdida de alimentación

Al volver la red eléctrica el programa arranca desde cero: sensores no validados, actuador apagado, sin ciclo en marcha. OTA a medias se comporta como describe [OTA.md](OTA.md). No hay memoria de "estaba calentando".

## Qué no afirmar

- V0.1 no limita temperatura.
- V0.1 no detecta puerta, sobrecorriente ni fallo del SSR.
- V0.1 no tiene segundo canal de medida.
- El log de reset no es un registro de accidentes: se pierde en el siguiente arranque y no se guarda en flash.
