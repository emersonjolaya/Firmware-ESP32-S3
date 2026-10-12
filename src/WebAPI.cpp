#include "WebAPI.h"
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "Sensores.h"
#include "MQ135.h"
#include "Microfono.h"
#include "Motores.h"

extern WebServer server;  // definido en el .ino principal

void webApiAddCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// GET /api/sensores — temperatura, humedad, gases (MQ135) y audio
void webApiHandleSensores() {
  sensoresActualizar();
  webApiAddCORSHeaders();

  const LecturaAmbiente& amb   = sensoresLectura();
  const LecturaGases&    gases = mq135Lectura();
  const LecturaAudio&    audio = microfonoLectura();

  StaticJsonDocument<768> doc;
  doc["temperatura"] = amb.temperatura;
  doc["humedad"]     = amb.humedad;
  doc["valido"]      = amb.valido;
  doc["uptime"]      = millis() / 1000;

  JsonObject mq = doc.createNestedObject("mq135");
  mq["estado"]  = mq135EstadoTexto();
  mq["co2"]     = gases.co2;
  mq["co"]      = gases.co;
  mq["nh3"]     = gases.nh3;
  mq["alcohol"] = gases.alcohol;
  mq["tolueno"] = gases.tolueno;
  mq["acetona"] = gases.acetona;
  mq["rs"]      = gases.rs;
  mq["vAout"]   = gases.vAout;

  JsonObject aud = doc.createNestedObject("audio");
  aud["rmsMv"]      = audio.rmsMv;
  aud["picoMv"]     = audio.picoMv;
  aud["nivelDb"]    = audio.nivelDb;
  aud["unidad"]     = "dB_rel";
  aud["gananciaDb"] = microfonoGananciaDb();
  aud["offsetMv"]   = audio.offsetMv;
  aud["fsHz"]       = audio.frecuenciaHz;
  aud["valida"]     = audio.valida;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// GET /api/estado — estado general del dispositivo
void webApiHandleEstado() {
  webApiAddCORSHeaders();

  StaticJsonDocument<256> doc;
  doc["dispositivo"] = "BioRed ESP32";
  doc["version"]     = "3.1.0";
  doc["ip"]          = WiFi.localIP().toString();
  doc["rssi"]        = WiFi.RSSI();
  doc["uptime"]      = millis() / 1000;
  doc["ssid"]        = WiFi.SSID();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// GET /api/mover?dir=<ARRIBA|ABAJO|IZQ|DER>&pasos=<N>
void webApiHandleMover() {
  webApiAddCORSHeaders();

  if (!server.hasArg("dir")) {
    server.send(400, "application/json", "{\"error\":\"Falta parametro dir\"}");
    return;
  }

  String dir = server.arg("dir");
  int pasos  = server.hasArg("pasos") ? server.arg("pasos").toInt() : motoresPasosPorDefecto();

  String error;
  bool ok = motoresMover(dir, pasos, error);

  StaticJsonDocument<128> resp;
  resp["dir"]   = dir;
  resp["pasos"] = pasos;
  resp["ok"]    = ok;
  if (!ok) resp["error"] = error;

  String json;
  serializeJson(resp, json);
  server.send(ok ? 200 : 400, "application/json", json);
}

// OPTIONS /* — pre-flight CORS
void webApiHandleOptions() {
  webApiAddCORSHeaders();
  server.send(204);
}
