#include "Motores.h"
#include <Stepper.h>

namespace {
  const int PASOS_POR_REVOLUCION = 2048;
  const int VELOCIDAD_RPM        = 5;
  const int PASOS_POR_PULSO      = 20;

  Stepper motorPan(PASOS_POR_REVOLUCION, 48, 21, 47, 20);
  Stepper motorTilt(PASOS_POR_REVOLUCION, 1, 42, 2, 41);

  // Apaga las 4 bobinas de un motor para que no consuma corriente quieto.
  // Deshabilitado a proposito: dejar las bobinas energizadas mantiene el par
  // de sujecion, evitando que el motor se mueva solo por gravedad o un golpe
  // externo. Descomentar si se prefiere ahorrar corriente sobre mantener posicion.
  void liberarMotor(int a, int b, int c, int d) {
    digitalWrite(a, LOW); digitalWrite(b, LOW);
    digitalWrite(c, LOW); digitalWrite(d, LOW);
  }
}

void motoresInit() {
  motorPan.setSpeed(VELOCIDAD_RPM);
  motorTilt.setSpeed(VELOCIDAD_RPM);
}

int motoresPasosPorDefecto() {
  return PASOS_POR_PULSO;
}

bool motoresMover(const String& dir, int& pasos, String& error) {
  pasos = constrain(pasos, 1, 200);  // limita bloqueos largos del servidor

  if      (dir == "ARRIBA") motorTilt.step( pasos);
  else if (dir == "ABAJO")  motorTilt.step(-pasos);
  else if (dir == "IZQ")    motorPan.step( pasos);
  else if (dir == "DER")    motorPan.step(-pasos);
  else {
    error = "Dirección inválida. Usa ARRIBA, ABAJO, IZQ o DER";
    return false;
  }

  // Deshabilitado junto con liberarMotor() arriba: mantiene el par de
  // sujecion del motor que se acaba de mover en vez de liberarlo.
  if (dir == "ARRIBA" || dir == "ABAJO") liberarMotor(1, 42, 2, 41);   // tilt
  else                                    liberarMotor(48, 21, 47, 20); // pan

  Serial.printf("[Motor] %s %d pasos\n", dir.c_str(), pasos);
  return true;
}
