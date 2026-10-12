#pragma once
#include <Arduino.h>

// Microfono MAX9814 en GPIO19 (ADC2_CH8). Muestreo no bloqueante a 8 kHz.
// Nota: GPIO19 es ADC2 -> con WiFi activo puede haber lecturas erraticas
// ocasionales (limitacion de hardware del ESP32).

struct LecturaAudio {
  float    rmsMv, picoMv, nivelDb, offsetMv;
  uint32_t frecuenciaHz;
  bool     valida;
  uint32_t timestamp;
};

void microfonoInit();

// Llamar en cada loop(). No bloquea.
void microfonoActualizar();

const LecturaAudio& microfonoLectura();

float microfonoGananciaDb();
