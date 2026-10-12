#pragma once

// Camara FreeNove ESP32-S3-WROOM CAM (OV2640/OV3660/OV5640).
// Pines fijos por el conector de la placa: 4,5,6,7,8,9,10,11,12,13,15,16,17,18.

// Inicializa la camara. Debe llamarse despues de Serial.begin().
// Si falla, se queda bloqueado parpadeando el error (evita reinicios en bucle).
void camaraInit();

// Handlers para registrar con WebServer::on(...)
void camaraHandleJpgLo();
void camaraHandleJpgHi();
void camaraHandleStream();
