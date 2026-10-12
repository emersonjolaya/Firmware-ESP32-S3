#include "Microfono.h"

namespace {
  const int   MIC_PIN            = 19;  // ADC2_CH8
  const float GANANCIA_MIC_DB    = 50.0f;  // GAIN a GND = 50 dB

  const uint32_t PERIODO_MUESTRA_US         = 125;  // 8 kHz
  const uint32_t DURACION_VENTANA_MS        = 50;
  const uint32_t INTERVALO_REPORTE_AUDIO_MS = 3000;

  const float OFFSET_MIN_MV = 1000.0f;  // rango esperado del offset DC (1.25 V nominal)
  const float OFFSET_MAX_MV = 1500.0f;
  const float RMS_MINIMO_MV = 0.1f;     // evita log10 de cero

  struct VentanaAudio {
    double   suma, sumaCuadrados;
    uint32_t minimo, maximo, muestras;
    uint32_t inicioMs;
  };

  struct IntervaloAudio {
    double   sumaRmsCuadrado, sumaOffset;
    float    picoMaximo;
    uint32_t ventanas, muestras;
    uint32_t inicioMs;
  };

  VentanaAudio   ventana;
  IntervaloAudio intervalo;
  LecturaAudio   lectura;
  uint32_t       proximaMuestraUs = 0;

  void reiniciarVentana() {
    ventana.suma          = 0;
    ventana.sumaCuadrados = 0;
    ventana.minimo        = UINT32_MAX;
    ventana.maximo        = 0;
    ventana.muestras      = 0;
    ventana.inicioMs      = millis();
  }

  void reiniciarIntervalo() {
    intervalo.sumaRmsCuadrado = 0;
    intervalo.sumaOffset      = 0;
    intervalo.picoMaximo      = 0;
    intervalo.ventanas        = 0;
    intervalo.muestras        = 0;
    intervalo.inicioMs        = millis();
  }

  // Toma una muestra solo cuando le toca (no bloquea)
  void tomarMuestra() {
    uint32_t ahoraUs = micros();
    if ((int32_t)(ahoraUs - proximaMuestraUs) < 0) return;

    if (ahoraUs - proximaMuestraUs > PERIODO_MUESTRA_US * 10) {
      proximaMuestraUs = ahoraUs;  // el loop se atraso, resincroniza
    }
    proximaMuestraUs += PERIODO_MUESTRA_US;

    uint32_t mv = analogReadMilliVolts(MIC_PIN);

    ventana.suma          += mv;
    ventana.sumaCuadrados += (double)mv * mv;
    if (mv < ventana.minimo) ventana.minimo = mv;
    if (mv > ventana.maximo) ventana.maximo = mv;
    ventana.muestras++;
  }

  void cerrarVentana() {
    if (millis() - ventana.inicioMs < DURACION_VENTANA_MS) return;
    if (ventana.muestras == 0) { reiniciarVentana(); return; }

    double media    = ventana.suma / ventana.muestras;  // offset DC
    double varianza = ventana.sumaCuadrados / ventana.muestras - media * media;
    if (varianza < 0) varianza = 0;
    float rms  = sqrt(varianza);
    float pico = (float)(ventana.maximo - ventana.minimo);

    intervalo.sumaRmsCuadrado += (double)rms * rms;
    intervalo.sumaOffset      += media;
    if (pico > intervalo.picoMaximo) intervalo.picoMaximo = pico;
    intervalo.ventanas++;
    intervalo.muestras += ventana.muestras;

    reiniciarVentana();
  }

  void consolidar() {
    if (millis() - intervalo.inicioMs < INTERVALO_REPORTE_AUDIO_MS) return;
    if (intervalo.ventanas == 0) { reiniciarIntervalo(); return; }

    uint32_t duracionMs = millis() - intervalo.inicioMs;
    float rms = sqrt(intervalo.sumaRmsCuadrado / intervalo.ventanas);

    lectura.rmsMv        = rms;
    lectura.picoMv       = intervalo.picoMaximo;
    lectura.nivelDb      = 20.0 * log10(fmax(rms, RMS_MINIMO_MV) / 1.0);
    lectura.offsetMv     = intervalo.sumaOffset / intervalo.ventanas;
    lectura.frecuenciaHz = (uint32_t)(intervalo.muestras * 1000.0 / duracionMs);
    lectura.valida       = lectura.offsetMv >= OFFSET_MIN_MV && lectura.offsetMv <= OFFSET_MAX_MV;
    lectura.timestamp    = millis();

    Serial.printf("[Audio] RMS=%.1f mV  Pico=%.0f mV  Nivel=%.1f dB_rel  Offset=%.0f mV  Fs=%lu Hz\n",
                  lectura.rmsMv, lectura.picoMv, lectura.nivelDb,
                  lectura.offsetMv, (unsigned long)lectura.frecuenciaHz);

    if (!lectura.valida) {
      Serial.printf("[Audio] AVISO offset fuera de rango (%.0f mV, esperado ~1250 mV). Revisa cableado en GPIO%d\n",
                    lectura.offsetMv, MIC_PIN);
    }

    reiniciarIntervalo();
  }
}

void microfonoInit() {
  reiniciarVentana();
  reiniciarIntervalo();
  proximaMuestraUs = micros();
  Serial.printf("[Audio] Microfono en GPIO%d | Ganancia %.0f dB | Muestreo %lu Hz\n",
                MIC_PIN, GANANCIA_MIC_DB, (unsigned long)(1000000UL / PERIODO_MUESTRA_US));
}

void microfonoActualizar() {
  tomarMuestra();
  cerrarVentana();
  consolidar();
}

const LecturaAudio& microfonoLectura() {
  return lectura;
}

float microfonoGananciaDb() {
  return GANANCIA_MIC_DB;
}
