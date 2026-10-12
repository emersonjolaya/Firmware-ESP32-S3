#include "MQ135.h"
#include <MQUnifiedsensor.h>

namespace {
  const int   MQ_PIN             = 3;
  const float MQ_SENSOR_VCC      = 5.0f;
  const float MQ_ADC_VREF        = 3.3f;  // referencia real del ADC del ESP32-S3
  const int   MQ_ADC_RESOLUTION  = 12;
  const float MQ_RATIO_CLEAN_AIR = 3.6f;

  const uint32_t MQ_WARMUP_MS         = 180000UL;  // 3 min
  const uint32_t MQ_CALIB_INTERVAL_MS = 200;
  const int      MQ_CALIB_SAMPLES     = 30;
  const uint32_t MQ_LOOP_INTERVAL_MS  = 2000;

  // La libreria usa "Voltage_Resolution" tanto para convertir el ADC a
  // voltios como para el Vcc de la formula de RS. Como aqui son distintos
  // (ADC a 3.3V, sensor a 5V), se construye con MQ_SENSOR_VCC y se le pasa
  // el voltaje ya convertido via externalADCUpdate() en vez de update().
  MQUnifiedsensor mq135Sensor("ESP32", MQ_SENSOR_VCC, MQ_ADC_RESOLUTION, MQ_PIN, "MQ-135");

  enum Estado { CALENTANDO, CALIBRANDO, LISTO };
  Estado        estado        = CALENTANDO;
  unsigned long inicioEstado  = 0;
  unsigned long ultimaMuestra = 0;
  unsigned long ultimoLog     = 0;
  int           muestraActual = 0;
  float         r0Acumulado   = 0;

  LecturaGases lectura = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  float leerVoltaje() {
    return analogRead(MQ_PIN) * (MQ_ADC_VREF / 4095.0f);
  }

  // RS = RL * (Vcc - V_aout) / V_aout, con RL = 10 kO (modulo tipico).
  float calcularRS(float v_aout) {
    const float RL = 10.0f;
    if (v_aout < 0.01f) return 0.0f;
    return RL * ((MQ_SENSOR_VCC - v_aout) / v_aout);
  }
}

void mq135Init() {
  estado       = CALENTANDO;
  inicioEstado = millis();
  ultimoLog    = millis();
  Serial.println("[MQ135] Calentando sensor (3 min, no bloqueante)...");
}

const char* mq135EstadoTexto() {
  switch (estado) {
    case CALENTANDO: return "calentando";
    case CALIBRANDO: return "calibrando";
    default:          return "listo";
  }
}

void mq135Actualizar() {
  switch (estado) {

    case CALENTANDO:
      if (millis() - ultimoLog >= 15000) {
        ultimoLog = millis();
        unsigned long restantes = (MQ_WARMUP_MS - (millis() - inicioEstado)) / 1000;
        Serial.printf("[MQ135] Calentando... %lu s restantes\n", restantes);
      }
      if (millis() - inicioEstado >= MQ_WARMUP_MS) {
        Serial.println("[MQ135] Calentamiento completo. Calibrando R0 (aire limpio)...");
        mq135Sensor.setRegressionMethod(1);  // exponencial: ppm = A * (RS/R0)^B
        mq135Sensor.init();
        estado        = CALIBRANDO;
        muestraActual = 0;
        r0Acumulado   = 0;
        ultimaMuestra = 0;
      }
      break;

    case CALIBRANDO:
      if (millis() - ultimaMuestra >= MQ_CALIB_INTERVAL_MS) {
        ultimaMuestra = millis();
        mq135Sensor.externalADCUpdate(leerVoltaje());
        r0Acumulado += mq135Sensor.calibrate(MQ_RATIO_CLEAN_AIR);
        muestraActual++;

        if (muestraActual >= MQ_CALIB_SAMPLES) {
          float r0Final = r0Acumulado / MQ_CALIB_SAMPLES;
          mq135Sensor.setR0(r0Final);
          Serial.printf("[MQ135] R0 calibrado = %.4f kO\n", r0Final);
          if (r0Final < 1.0f || r0Final > 100.0f) {
            Serial.println("[MQ135] AVISO R0 fuera de rango (1-100 kO). Revisar alimentacion 5V y ambiente sin gases.");
          }
          estado = LISTO;
        }
      }
      break;

    case LISTO: {
      static unsigned long ultimaLectura = 0;
      if (millis() - ultimaLectura < MQ_LOOP_INTERVAL_MS) break;
      ultimaLectura = millis();

      lectura.adcRaw = analogRead(MQ_PIN);
      lectura.vAout  = leerVoltaje();
      lectura.rs     = calcularRS(lectura.vAout);

      // Coeficientes A/B por gas (datasheet MQ135, 20°C/65%HR). Valores
      // orientativos: el sensor mide conductividad total, no gases individuales.
      mq135Sensor.externalADCUpdate(lectura.vAout);
      mq135Sensor.setA(110.47f); mq135Sensor.setB(-2.862f);  lectura.co2     = mq135Sensor.readSensor();
      mq135Sensor.setA(605.18f); mq135Sensor.setB(-3.937f);  lectura.co      = mq135Sensor.readSensor();
      mq135Sensor.setA(102.2f);  mq135Sensor.setB(-2.473f);  lectura.nh3     = mq135Sensor.readSensor();
      mq135Sensor.setA(77.255f); mq135Sensor.setB(-3.18f);   lectura.alcohol = mq135Sensor.readSensor();
      mq135Sensor.setA(44.947f); mq135Sensor.setB(-3.445f);  lectura.tolueno = mq135Sensor.readSensor();
      mq135Sensor.setA(34.668f); mq135Sensor.setB(-3.369f);  lectura.acetona = mq135Sensor.readSensor();
      lectura.timestamp = millis();

      Serial.printf("[MQ135] CO2=%.1f CO=%.1f NH3=%.1f Alcohol=%.1f Tolueno=%.1f Acetona=%.1f ppm\n",
                    lectura.co2, lectura.co, lectura.nh3, lectura.alcohol, lectura.tolueno, lectura.acetona);
      break;
    }
  }
}

const LecturaGases& mq135Lectura() {
  return lectura;
}
