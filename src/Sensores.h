#pragma once
#include <Arduino.h>

// DHT22 (temperatura / humedad ambiente).

struct LecturaAmbiente {
  float temperatura;
  float humedad;
  bool  valido;
  unsigned long timestamp;
};

void sensoresInit();

// Llamar en cada loop(). Internamente se limita a leer cada 3 s.
void sensoresActualizar();

const LecturaAmbiente& sensoresLectura();
