#pragma once
#include <Arduino.h>

// Motores paso a paso (pan/tilt) de la camara.

void motoresInit();

// Mueve el motor segun dir ("ARRIBA"|"ABAJO"|"IZQ"|"DER"). pasos se
// recibe por referencia y queda ajustado (1-200) con el valor realmente
// usado. Devuelve false y llena error si dir es invalido.
bool motoresMover(const String& dir, int& pasos, String& error);

int motoresPasosPorDefecto();
