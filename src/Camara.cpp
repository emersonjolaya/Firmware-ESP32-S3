#include "Camara.h"
#include <WiFi.h>
#include <WebServer.h>
#include <esp32cam.h>
#include <esp_camera.h>

extern WebServer server;  // definido en el .ino principal

namespace {
  const esp32cam::Pins FreeNoveS3{
    /*D0*/ 11, /*D1*/ 9, /*D2*/ 8, /*D3*/ 10,
    /*D4*/ 12, /*D5*/ 18, /*D6*/ 17, /*D7*/ 16,
    /*XCLK*/ 15, /*PCLK*/ 13, /*VSYNC*/ 6, /*HREF*/ 7,
    /*SDA*/ 4, /*SCL*/ 5, /*RESET*/ -1, /*PWDN*/ -1
  };

  auto camLoRes     = esp32cam::Resolution::find(320, 240);  // baja resolucion
  auto camHiRes     = esp32cam::Resolution::find(800, 600);  // alta resolucion
  auto camStreamRes = esp32cam::Resolution::find(320, 240);  // resolucion del stream

  esp32cam::Resolution camResActual = camHiRes;  // evita reconfigurar si no cambio

  void serveJpg(const esp32cam::Resolution& res) {
    if (!(res == camResActual)) {
      if (!esp32cam::Camera.changeResolution(res)) {
        Serial.println("[Camara] SET-RES FAIL");
      }
      camResActual = res;
    }

    auto frame = esp32cam::capture();
    if (frame == nullptr) {
      Serial.println("[Camara] CAPTURE FAIL");
      server.send(503, "", "");
      return;
    }

    server.setContentLength(frame->size());
    server.send(200, "image/jpeg");
    WiFiClient client = server.client();
    frame->writeTo(client);
  }
}

void camaraInit() {
  Serial.printf("[Camara] PSRAM: %s (%u bytes)\n", psramFound() ? "OK" : "NO", (unsigned)ESP.getPsramSize());
  esp32cam::setLogger(Serial);

  using namespace esp32cam;
  Config cfg;
  cfg.setPins(FreeNoveS3);
  cfg.setResolution(camHiRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "[Camara] CAMARA OK" : "[Camara] CAMARA FAIL");
  sensor_t* s = esp_camera_sensor_get();
  if (s != nullptr) {
    Serial.printf("[Camara] Sensor PID: 0x%04x\n", s->id.PID);  // 0x26=OV2640, 0x3660=OV3660, 0x5640=OV5640
  } else {
    Serial.println("[Camara] Sensor NO DETECTADO");
  }
  if (!ok) {
    Serial.println("[Camara] Revisa pines y Tools > PSRAM = OPI PSRAM");
    while (true) delay(1000);  // evita reinicios en bucle
  }
}

void camaraHandleJpgLo() { serveJpg(camLoRes); }
void camaraHandleJpgHi() { serveJpg(camHiRes); }

void camaraHandleStream() {
  WiFiClient client = server.client();

  if (!(camStreamRes == camResActual)) {
    esp32cam::Camera.changeResolution(camStreamRes);
    camResActual = camStreamRes;
  }

  client.print("HTTP/1.1 200 OK\r\n"
               "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
               "Cache-Control: no-cache\r\n"
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    auto frame = esp32cam::capture();
    if (frame == nullptr) { delay(10); continue; }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                  static_cast<unsigned>(frame->size()));
    if (!frame->writeTo(client)) break;  // cliente desconectado
    client.print("\r\n");
  }
  Serial.println("[Camara] Stream cerrado");
}
