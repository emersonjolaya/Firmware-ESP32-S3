#include "Sensores.h"
#include <DHT.h>

namespace {
  const int DHTPIN = 14;
  const unsigned long INTERVALO_LECTURA_MS = 3000;

  DHT dht(DHTPIN, DHT22);
  LecturaAmbiente lectura = {0, 0, false, 0};
}

void sensoresInit() {
  dht.begin();
  // Timestamp retrasado para forzar una primera lectura inmediata.
  lectura.timestamp = millis() - INTERVALO_LECTURA_MS;
}

void sensoresActualizar() {
  unsigned long ahora = millis();
  if (ahora - lectura.timestamp < INTERVALO_LECTURA_MS) return;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  lectura.valido      = !(isnan(h) || isnan(t));
  lectura.temperatura = lectura.valido ? t : -1.0;
  lectura.humedad     = lectura.valido ? h : -1.0;
  lectura.timestamp   = ahora;

  Serial.printf("[Sensores] T=%.1f°C  H=%.1f%%\n", lectura.temperatura, lectura.humedad);
}

const LecturaAmbiente& sensoresLectura() {
  return lectura;
}
