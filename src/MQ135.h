#pragma once
#include <Arduino.h>

// Sensor de gases MQ135. A-out en GPIO3, sin divisor de tension.
// Calentamiento (3 min) y calibracion de R0 no bloqueantes: se avanzan
// solos en mq135Actualizar(), llamado en cada loop().

struct LecturaGases {
  float co2, co, nh3, alcohol, tolueno, acetona;
  float rs, vAout;
  int   adcRaw;
  unsigned long timestamp;
};

// Arranca el calentamiento. Llamar una vez en setup().
void mq135Init();

// Avanza la maquina de estados (calentando -> calibrando -> listo) y,
// una vez listo, actualiza la lectura de gases cada 2 s. Llamar en loop().
void mq135Actualizar();

// "calentando" | "calibrando" | "listo"
const char* mq135EstadoTexto();

const LecturaGases& mq135Lectura();
