#pragma once
#include <Arduino.h>

// Envio de las lecturas a la API de BioRed (POST /api/v1/readings/ingest).
// Toma las lecturas de los modulos Sensores, MQ135 y Microfono, arma el JSON
// del contrato de ingesta (README de BioRed_DB_v3 y coleccion de Postman) y
// lo envia por HTTPS con la key de dispositivo cada 30 s.
//
// Corre en su propia tarea de FreeRTOS en el nucleo 0. Asi el handshake TLS
// (1-3 s) no bloquea loop(): el servidor web, el muestreo del microfono y el
// MQ135 siguen funcionando mientras se envia.
//
// URL, key, intervalo y version del firmware estan en EnvioAPIConfig.h.

struct EstadoEnvioAPI {
  uint32_t enviados;       // lecturas guardadas (201)
  uint32_t fallidos;       // envios con error
  int      ultimoCodigo;   // ultimo codigo HTTP (negativo = error de conexion)
  uint32_t ultimoEnvioMs;  // millis() del ultimo intento
};

// Crea la tarea de envio. Llamar una vez en setup(), despues de conectar el WiFi.
void envioApiInit();

const EstadoEnvioAPI& envioApiEstado();
