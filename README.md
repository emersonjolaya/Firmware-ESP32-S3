# ESP32-S3-FW-BioRed

Firmware modular de la estación BioRed para la placa ESP32-S3 WROOM N16R8 (Freenove). Mide temperatura y humedad con el DHT22, gases con el MQ-135 y sonido con el MAX9814. También transmite la cámara, mueve el soporte pan/tilt, sirve un dashboard en la red local y envía cada lectura a la API de BioRed en `https://biored-analytic.brayan36.com`.

Este README explica cómo poner en marcha una estación nueva con el mismo firmware. El registro de la estación en el servidor está documentado en `docs/registro-estacion.md` del repositorio [biored-analytic-app](https://github.com/Brayan36/biored-analytic-app).

## Estructura

```
ESP32-S3-FW-BioRed.ino          orquesta WiFi, LittleFS, rutas, setup() y loop()
src/
  Sensores.h / .cpp             DHT22 (temperatura y humedad), lectura cada 3 s
  MQ135.h / .cpp                MQ-135, calentamiento de 3 min, calibración de R0 y 6 gases
  Microfono.h / .cpp            MAX9814, muestreo a 8 kHz y consolidación cada 3 s
  Camara.h / .cpp               cámara OV2640/OV3660/OV5640, fotos y stream
  Motores.h / .cpp              motores paso a paso pan/tilt
  WebAPI.h / .cpp               API local /api/sensores, /api/estado y /api/mover
  EnvioAPI.h / .cpp             envío de las lecturas a la API de BioRed cada 30 s
  EnvioAPIConfig.ejemplo.h      plantilla de configuración del envío (se copia como EnvioAPIConfig.h)
data/                           dashboard local (index.htm, style.css, app.js) que se sube a LittleFS
```

Cada módulo expone sus funciones en el `.h` y guarda su lógica y sus constantes en el `.cpp`. El `.ino` solo inicializa los módulos en `setup()` y los actualiza en `loop()`.

## Pines

| Componente | Pines |
|---|---|
| DHT22 | 14 |
| MQ-135 (A-out) | 3 (ADC1_CH2, sin divisor de voltaje) |
| MAX9814 (OUT) | 19 (ADC2_CH8) |
| Motor tilt | 1, 42, 2, 41 |
| Motor pan | 48, 21, 47, 20 |
| Cámara | 4 a 13, 15 a 18 (fijos por el conector de la placa) |

El LDR no está en esta versión. Usaba el GPIO34, que no existe en el ESP32-S3. Además no queda un pin ADC libre para reubicarlo.

## Requisitos

**Arduino IDE**

| Opción en Herramientas | Valor |
|---|---|
| Placa | ESP32S3 Dev Module (paquete esp32 de Espressif) |
| PSRAM | OPI PSRAM |
| Partition Scheme | Uno de 16 MB con espacio para LittleFS |

**Librerías** (Gestor de librerías)

| Librería | Autor | Uso |
|---|---|---|
| DHT sensor library | Adafruit | DHT22. Pide instalar también Adafruit Unified Sensor |
| MQUnifiedsensor | Miguel Califa | MQ-135 |
| ArduinoJson | Benoit Blanchon | JSON de la API local y del envío |
| esp32cam | yoursunny | Cámara |

`WiFi`, `WebServer`, `LittleFS`, `HTTPClient`, `WiFiClientSecure` y `Stepper` vienen con el paquete de la placa.

**Plugin** `arduino-littlefs-upload`, para subir la carpeta `data/` con el dashboard local.

## Puesta en marcha de una estación nueva

1. **Clonar el repositorio** y abrir `ESP32-S3-FW-BioRed.ino` en el Arduino IDE.

2. **Configurar el wifi.** En el `.ino`, cambiar `WIFI_SSID` y `WIFI_PASS` por los de la red donde estará la estación. Este cambio es local y no se sube al repositorio.

3. **Pedir la key de dispositivo** al administrador del servidor. Cada placa tiene su propia key, así se puede revocar una sin afectar a las demás.

4. **Crear el archivo de configuración del envío.** En la carpeta `src/`, copiar `EnvioAPIConfig.ejemplo.h` con el nombre `EnvioAPIConfig.h` y reemplazar `PEGA_AQUI_LA_KEY_DE_DISPOSITIVO` por la key del paso 3. En Windows (PowerShell, desde la carpeta del proyecto)

   ```powershell
   Copy-Item src\EnvioAPIConfig.ejemplo.h src\EnvioAPIConfig.h
   ```

   `EnvioAPIConfig.h` está en `.gitignore` para que la key nunca llegue al repositorio.

5. **Compilar y subir** el sketch. La primera vez, subir también la carpeta `data/` con **Upload LittleFS**.

6. **Obtener la MAC.** Abrir el monitor serie a 115200 baudios. Al arrancar aparece esta línea con la MAC de la placa

   ```
   [EnvioAPI] Enviando cada 30 s a https://biored-analytic.brayan36.com/... como AA:BB:CC:DD:EE:FF
   ```

   Mientras la estación no esté registrada, el envío responde `Error 422`, que es lo esperado.

7. **Registrar la estación en el servidor** con la colección de Postman de biored-analytic-app (key de administración). Se crea la estación con la MAC del paso 6 y se instalan sus tres sensores con los pines de este firmware.

   | Modelo | metadata |
   |---|---|
   | DHT22 | `{ "pin": 14 }` |
   | MQ135 | `"pin": 3` y las curvas de `docs/registro-estacion.md` |
   | MAX9814 | `{ "pin": 19, "gain_db": 50, "ar_ratio": "1:4000", "unit": "dB_rel", "target_sample_rate_hz": 8000 }` |

   Los tres son obligatorios porque el módulo envía las tres secciones. El LDR no se registra.

8. **Reiniciar la placa.** En el monitor serie debe aparecer `[EnvioAPI] 201 lectura N guardada` cada 30 s. La estación también aparece en `https://biored-analytic.brayan36.com/locaciones`.

## Cómo funciona el módulo EnvioAPI

- **Se activa con dos líneas del `.ino`.** `#include "src/EnvioAPI.h"` junto a los demás includes y `envioApiInit();` al final de `setup()`, después de conectar el wifi.
- **Obtiene la MAC sola.** `WiFi.macAddress()` lee la MAC de fábrica del chip, que es única por placa. Se envía en mayúsculas como `device_uid` en cada lectura. Por eso `envioApiInit()` va después del wifi, porque antes la MAC puede salir en ceros.
- **No lee los sensores por su cuenta.** Copia las lecturas que ya tienen los módulos Sensores, MQ135 y Microfono. Con ellas arma el JSON del contrato de ingesta (`POST /api/v1/readings/ingest`).
- **No bloquea el firmware.** Corre en una tarea de FreeRTOS en el núcleo 0, así la conexión HTTPS no detiene la cámara, el dashboard ni el muestreo del micrófono.
- **Valida el HTTPS.** Incluye los certificados raíz de Google Trust Services, Let's Encrypt y SSL.com, las autoridades que usa Cloudflare. No usa `setInsecure()`.
- **Respeta el calentamiento del MQ-135.** Durante los primeros 3 minutos envía `warmed_up: false` y la API guarda la lectura sin gases.

Ejemplo de un envío completo

```json
{
  "device_uid": "80:B5:4E:C7:50:94",
  "firmware_version": "3.1.0-modular",
  "uptime_s": 3600,
  "dht22": { "valid": true, "temperature_c": 22.6, "humidity_pct": 61.8 },
  "mq135": {
    "warmed_up": true, "raw": 1340, "output_voltage": 1.08, "rs_kohm": 36.296, "alarm": null,
    "estimates": [
      { "gas": "CO2", "ppm": 1.66 }, { "gas": "CO", "ppm": 1.88 }, { "gas": "NH3", "ppm": 2.1 },
      { "gas": "ALCOHOL", "ppm": 0.73 }, { "gas": "TOLUENE", "ppm": 0.3 }, { "gas": "ACETONE", "ppm": 0.25 }
    ]
  },
  "audio": { "valid": true, "rms_mv": 24.1, "peak_to_peak_mv": 180, "offset_mv": 1230.5, "sample_rate_hz": 7980 }
}
```

## Configuración del envío (`src/EnvioAPIConfig.h`)

| Constante | Valor por defecto | Para qué sirve |
|---|---|---|
| `URL` | `https://biored-analytic.brayan36.com/api/v1/readings/ingest` | Endpoint de ingesta. Siempre con `https://` |
| `API_KEY` | (la de cada placa) | Key de dispositivo, nunca la de administración |
| `DEVICE_UID` | `""` | Vacío usa la MAC del chip. Solo se llena para que una placa de reemplazo conserve la identidad de la anterior |
| `FIRMWARE_VERSION` | `3.1.0-modular` | Queda guardada en cada lectura |
| `INTERVALO_MS` | 30000 | Tiempo entre envíos |
| `ESPERA_MAXIMA_MS` | 300000 | Tope de la espera creciente ante 429, 5xx o falta de conexión |
| `ESPERA_ERROR_MS` | 300000 | Pausa tras 400, 401, 403 o 422, que se arreglan corrigiendo la configuración |
| `TIMEOUT_MS` | 10000 | Tiempo máximo de conexión y respuesta |
| `MOSTRAR_JSON` | `false` | `true` imprime en el monitor serie el JSON de cada envío |

## Monitor serie y errores

| Mensaje | Causa | Qué hacer |
|---|---|---|
| `201 lectura N guardada` | Todo funciona | Nada |
| `Falta la key de dispositivo` | `EnvioAPIConfig.h` conserva el texto de la plantilla | Pegar la key de la placa |
| `Error 401` | Key mal copiada o es la de administración | Revisar `API_KEY` |
| `Error 403` con HTML | Cloudflare bloqueó la petición | Regla del WAF para `/api/v1/readings/ingest` (ver `docs/despliegue.md` de biored-analytic-app) |
| `Error 422` | MAC no registrada o falta un sensor activo | Registrar la estación y sus tres sensores |
| `Error 400` | El JSON no cumple el contrato | Revisar la respuesta impresa (`error.details`) |
| `Error -1` o `Sin conexion` | Sin internet, dominio caído o certificado no válido | Revisar el wifi y `https://biored-analytic.brayan36.com/health` |
| `Sin WiFi, se omite este envio` | La placa perdió el wifi | Se reintenta en el siguiente ciclo |

## API local de la placa

Disponible en `http://IP-de-la-placa` dentro de la misma red.

| Ruta | Respuesta |
|---|---|
| `/` | Dashboard local desde LittleFS |
| `/api/sensores` | Última lectura de temperatura, humedad, gases y audio |
| `/api/estado` | Versión, IP, señal wifi y tiempo encendido |
| `/api/mover?dir=ARRIBA&pasos=20` | Mueve el pan/tilt (`ARRIBA`, `ABAJO`, `IZQ`, `DER`) |
| `/resources/cam/cam-lo.jpg`, `cam-hi.jpg`, `live` | Foto en baja o alta resolución y stream |

## Advertencias de hardware

- **MQ-135 sin divisor de voltaje.** La salida puede acercarse a 5 V y el ADC del ESP32-S3 solo admite 3,3 V. Se recomienda un divisor de 10 kΩ y 15 kΩ antes de un despliegue permanente.
- **Micrófono en ADC2.** Con el wifi activo pueden aparecer lecturas erráticas ocasionales. Es una limitación del ESP32.
- **Valores de gas orientativos.** El MQ-135 mide la conductividad total del aire y el firmware recalibra R0 en cada arranque, así que los ppm cambian de una sesión a otra.

## Seguridad

- No subir `src/EnvioAPIConfig.h` ni contraseñas de wifi al repositorio.
- Una key de dispositivo por placa, entregada por un canal privado.
- Si una key se filtra, el administrador borra su hash del servidor y genera otra.
