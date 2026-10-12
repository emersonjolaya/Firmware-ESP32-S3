/*
  ══════════════════════════════════════════════════════════════
   ESP32-S3-FW-BioRed — BioRed + Cámara + MQ135 + Micrófono
   Placa: ESP32-S3 WROOM N16R8 (Freenove)
  ══════════════════════════════════════════════════════════════

   Estructura
     ESP32-S3-FW-BioRed.ino     orquesta: WiFi, LittleFS, rutas, setup/loop
     src/                       un modulo .h/.cpp por funcionalidad
     data/                      dashboard (index.htm, style.css, app.js) —
                                 carpeta de trabajo real (nombre obligatorio
                                 para el plugin de subida de LittleFS), y
                                 editable/probable standalone en el navegador

   Mapa de pines
     motorTilt   1, 42, 2, 41
     motorPan    48, 21, 47, 20
     DHT22       14
     MQ135       3   (A-out, ADC1_CH2, sin divisor — ver advertencia abajo)
     Microfono   19  (MAX9814 OUT, ADC2_CH8)
     Camara      4,5,6,7,8,9,10,11,12,13,15,16,17,18 (FreeNoveS3, fijos por conector)

   Pendiente / fuera de esta version
     LDR eliminado: usaba GPIO34, que no existe en el ESP32-S3.
     No queda ningun pin ADC libre en la placa para reubicarlo.

   Advertencias de hardware
     - MQ135: A-out sin divisor de tension puede acercarse a 5V y saturar
       el ADC (max ~3.3V). Evaluar divisor si se ven lecturas en el tope.
     - Microfono en GPIO19 (ADC2): con WiFi activo pueden darse lecturas
       erraticas ocasionales (limitacion de hardware del ESP32).

   Requiere el plugin "arduino-littlefs-upload" para subir data/ al ESP32,
   y un Partition Scheme con espacio para LittleFS.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "src/Sensores.h"
#include "src/MQ135.h"
#include "src/Microfono.h"
#include "src/Camara.h"
#include "src/Motores.h"
#include "src/WebAPI.h"

const char* WIFI_SSID = "MontealegreRuiz_2.4G";
const char* WIFI_PASS = "079794DmSmBm";

WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== BioRed Firmware v3.1 (modular) ===");

  analogReadResolution(12);

  sensoresInit();
  motoresInit();
  mq135Init();      // arranca el calentamiento, no bloquea el resto del arranque
  microfonoInit();
  camaraInit();      // bloquea indefinidamente si la camara falla

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // menos lag en camara/motores
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✔ WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin(true)) {
    Serial.println("[FS] LittleFS FAIL — sube la carpeta data/ con 'Upload LittleFS' antes de abrir el dashboard");
  } else {
    Serial.println("[FS] LittleFS OK");
  }

  // Rutas API
  server.on("/api/sensores", HTTP_GET, webApiHandleSensores);
  server.on("/api/estado",   HTTP_GET, webApiHandleEstado);
  server.on("/api/mover",    HTTP_GET, webApiHandleMover);

  // Rutas camara
  server.on("/resources/cam/cam-lo.jpg", camaraHandleJpgLo);
  server.on("/resources/cam/cam-hi.jpg", camaraHandleJpgHi);
  server.on("/resources/cam/cam-lo",     camaraHandleJpgLo);
  server.on("/resources/cam/cam-hi",     camaraHandleJpgHi);
  server.on("/resources/cam/live",       camaraHandleStream);

  // Dashboard estatico (index.htm, style.css, app.js) servido desde LittleFS
  server.serveStatic("/", LittleFS, "/");

  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) { webApiHandleOptions(); }
    else {
      webApiAddCORSHeaders();
      server.send(404, "application/json", "{\"error\":\"Ruta no encontrada\"}");
    }
  });

  server.begin();
  Serial.println("✔ Servidor web iniciado");
}

void loop() {
  server.handleClient();
  sensoresActualizar();
  microfonoActualizar();
  mq135Actualizar();
}
