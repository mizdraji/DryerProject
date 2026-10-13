#pragma once

// Contrato para un actuador de potencia futuro (resistencia, ventilador).
// V0.1 no tiene implementacion. Quien lo implemente debe cumplir:
//   - begin() y el estado de reposo dejan el actuador en OFF;
//   - forceOff() es inmediato y no consulta una bandera guardada;
//   - un reinicio no puede encender el actuador solo porque "estaba en marcha".
class Actuator {
 public:
  virtual ~Actuator() = default;
  virtual void begin() = 0;
  virtual void forceOff() = 0;
  virtual void setEnabled(bool enabled) = 0;
  virtual bool isEnabled() const = 0;
};
