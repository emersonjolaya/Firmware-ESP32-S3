#pragma once

// PLANTILLA de configuracion del modulo EnvioAPI.
// 1. Copia este archivo en la misma carpeta con el nombre EnvioAPIConfig.h
// 2. En la copia reemplaza PEGA_AQUI_LA_KEY_DE_DISPOSITIVO por la key de tu placa
// EnvioAPIConfig.h esta en .gitignore. Nunca subas la key al repositorio.

namespace EnvioAPIConfig {
  // Endpoint de ingesta (alias equivalente: /api/v1/lecturas). Siempre https://
  const char* const URL = "https://biored-analytic.brayan36.com/api/v1/readings/ingest";

  // Key de DISPOSITIVO de esta placa (una por ESP32), no la de administracion
  const char* const API_KEY = "PEGA_AQUI_LA_KEY_DE_DISPOSITIVO";

  // MAC registrada de la estacion. Vacio = usa WiFi.macAddress() del propio ESP32
  const char* const DEVICE_UID = "";

  // Version que queda guardada en cada lectura (maximo 20 caracteres)
  const char* const FIRMWARE_VERSION = "3.1.0-modular";

  const uint32_t INTERVALO_MS       = 30000UL;   // un envio cada 30 s
  const uint32_t ESPERA_MAXIMA_MS   = 300000UL;  // tope del reintento con espera creciente (5 min)
  const uint32_t ESPERA_ERROR_MS    = 300000UL;  // pausa tras 400/401/403/422 (error de configuracion)
  const uint32_t TIMEOUT_MS         = 10000UL;   // tiempo maximo de conexion y respuesta

  // true = imprime en el monitor serie el JSON de cada envio (util para depurar)
  const bool MOSTRAR_JSON = false;
}
