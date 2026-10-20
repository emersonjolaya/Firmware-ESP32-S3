#include "EnvioAPI.h"
#include "EnvioAPIConfig.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>

#include "Sensores.h"
#include "MQ135.h"
#include "Microfono.h"

namespace {
  // Certificados raiz para validar el HTTPS del dominio (no se usa setInsecure).
  // Cloudflare emite el certificado con Google Trust Services, Let's Encrypt
  // o SSL.com y puede cambiar de autoridad al renovarlo, por eso van las raices
  // de las tres. mbedTLS acepta varios certificados en un mismo PEM.
  // Vencen entre 2035 y 2046.
  const char CERTIFICADOS_RAIZ[] = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICOjCCAcCgAwIBAgIQFAP1q/s3ixdAW+JDsqXRxDAKBggqhkjOPQQDAzBOMQsw
CQYDVQQGEwJVUzEYMBYGA1UECgwPU1NMIENvcnBvcmF0aW9uMSUwIwYDVQQDDBxT
U0wuY29tIFRMUyBFQ0MgUm9vdCBDQSAyMDIyMB4XDTIyMDgyNTE2MzM0OFoXDTQ2
MDgxOTE2MzM0N1owTjELMAkGA1UEBhMCVVMxGDAWBgNVBAoMD1NTTCBDb3Jwb3Jh
dGlvbjElMCMGA1UEAwwcU1NMLmNvbSBUTFMgRUNDIFJvb3QgQ0EgMjAyMjB2MBAG
ByqGSM49AgEGBSuBBAAiA2IABEUpNXP6wrgjzhR9qLFNoFs27iosU8NgCTWyJGYm
acCzldZdkkAZDsalE3D07xJRKF3nzL35PIXBz5SQySvOkkJYWWf9lCcQZIxPBLFN
SeR7T5v15wj4A4j3p8OSSxlUgaNjMGEwDwYDVR0TAQH/BAUwAwEB/zAfBgNVHSME
GDAWgBSJjy+j6CugFFR781a4Jl9nOAuc0DAdBgNVHQ4EFgQUiY8vo+groBRUe/NW
uCZfZzgLnNAwDgYDVR0PAQH/BAQDAgGGMAoGCCqGSM49BAMDA2gAMGUCMFXjIlbp
15IkWE8elDIPDAI2wv2sdDJO4fscgIijzPvX6yv/N33w7deedWo1dlJF4AIxAMeN
b0Igj762TVntd00pxCAgRWSGOlDGxK0tk/UYfXLtqc/ErFc2KAhl3zx5Zn6g6g==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICjTCCAhSgAwIBAgIIdebfy8FoW6gwCgYIKoZIzj0EAwIwfDELMAkGA1UEBhMC
VVMxDjAMBgNVBAgMBVRleGFzMRAwDgYDVQQHDAdIb3VzdG9uMRgwFgYDVQQKDA9T
U0wgQ29ycG9yYXRpb24xMTAvBgNVBAMMKFNTTC5jb20gUm9vdCBDZXJ0aWZpY2F0
aW9uIEF1dGhvcml0eSBFQ0MwHhcNMTYwMjEyMTgxNDAzWhcNNDEwMjEyMTgxNDAz
WjB8MQswCQYDVQQGEwJVUzEOMAwGA1UECAwFVGV4YXMxEDAOBgNVBAcMB0hvdXN0
b24xGDAWBgNVBAoMD1NTTCBDb3Jwb3JhdGlvbjExMC8GA1UEAwwoU1NMLmNvbSBS
b290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5IEVDQzB2MBAGByqGSM49AgEGBSuB
BAAiA2IABEVuqVDEpiM2nl8ojRfLliJkP9x6jh3MCLOicSS6jkm5BBtHllirLZXI
7Z4INcgn64mMU1jrYor+8FsPazFSY0E7ic3s7LaNGdM0B9y7xgZ/wkWV7Mt/qCPg
CemB+vNH06NjMGEwHQYDVR0OBBYEFILRhXMw5zUE044CkvvlpNHEIejNMA8GA1Ud
EwEB/wQFMAMBAf8wHwYDVR0jBBgwFoAUgtGFczDnNQTTjgKS++Wk0cQh6M0wDgYD
VR0PAQH/BAQDAgGGMAoGCCqGSM49BAMCA2cAMGQCMG/n61kRpGDPYbCWe+0F+S8T
kdzt5fxQaxFGRrMcIQBiu77D5+jNB5n5DQtdcj7EqgIwH7y6C+IwJPt8bYBVCpk+
gA0z5Wajs6O7pdWLjwkspl1+4vAHCGht0nxpbl/f5Wpl
-----END CERTIFICATE-----
)PEM";

  const uint32_t STACK_TAREA      = 16384;  // bytes. TLS necesita una pila amplia
  const uint32_t ESPERA_INICIAL_MS = 5000;  // da tiempo a la primera lectura de cada sensor

  EstadoEnvioAPI estado          = {0, 0, 0, 0};
  TaskHandle_t   tarea           = nullptr;
  char           deviceUid[18]   = "";
  uint8_t        fallosSeguidos  = 0;

  // loop() escribe las lecturas en el nucleo 1 mientras esta tarea las lee en
  // el nucleo 0. Se copia dos veces y se repite si cambio en medio, para no
  // mezclar los campos de dos lecturas distintas.
  template <typename T>
  T copiaEstable(const T& origen) {
    T a, b;
    for (int i = 0; i < 5; i++) {
      memcpy(&a, &origen, sizeof(T));
      __sync_synchronize();
      memcpy(&b, &origen, sizeof(T));
      if (memcmp(&a, &b, sizeof(T)) == 0) break;
    }
    return a;
  }

  // Limita al rango que acepta la API (las mismas restricciones CHECK de la BD)
  // y redondea. Un NaN se convierte en el minimo para no invalidar el envio.
  double numero(double x, double minimo, double maximo, int decimales) {
    if (!isfinite(x)) x = minimo;
    if (x < minimo) x = minimo;
    if (x > maximo) x = maximo;
    const double f = pow(10.0, decimales);
    return round(x * f) / f;
  }

  // gas_estimates.ppm es DECIMAL(10,2). La libreria devuelve 3.4e38 al
  // desbordarse: esas estimaciones se envian como null y la API las descarta.
  bool ppmGuardable(float ppm) {
    return isfinite(ppm) && ppm >= 0.0f && ppm <= 99999999.99f;
  }

  // Arma el JSON del contrato de ingesta. Cada seccion de sensor es opcional.
  String armarJson() {
    const LecturaAmbiente amb   = copiaEstable(sensoresLectura());
    const LecturaGases    gases = copiaEstable(mq135Lectura());
    const LecturaAudio    audio = copiaEstable(microfonoLectura());
    const bool mqListo = strcmp(mq135EstadoTexto(), "listo") == 0;

    StaticJsonDocument<1536> doc;  // el envio completo ocupa cerca de 900 bytes en el ESP32
    doc["device_uid"]       = deviceUid;
    doc["firmware_version"] = EnvioAPIConfig::FIRMWARE_VERSION;
    doc["uptime_s"]         = (uint32_t)(millis() / 1000);

    // DHT22. Si fallo, el modulo deja -1: se envia valid=false y la API guarda NULL.
    // Un valor fuera de rango tambien se marca invalido para que no rechacen todo el envio.
    const bool dhtValido = amb.valido &&
                           amb.temperatura >= -40.0f && amb.temperatura <= 80.0f &&
                           amb.humedad >= 0.0f && amb.humedad <= 100.0f;
    JsonObject dht = doc.createNestedObject("dht22");
    dht["valid"] = dhtValido;
    if (dhtValido) {
      dht["temperature_c"] = numero(amb.temperatura, -40, 80, 1);
      dht["humidity_pct"]  = numero(amb.humedad, 0, 100, 1);
    } else {
      dht["temperature_c"] = nullptr;
      dht["humidity_pct"]  = nullptr;
    }

    // MQ135. Mientras calienta o calibra R0 no se guarda gas (warmed_up=false).
    JsonObject mq = doc.createNestedObject("mq135");
    if (!mqListo || gases.timestamp == 0) {
      mq["warmed_up"] = false;
    } else {
      mq["warmed_up"]      = true;
      mq["raw"]            = (int)numero(gases.adcRaw, 0, 4095, 0);
      mq["output_voltage"] = numero(gases.vAout, 0, 5.5, 3);
      if (isfinite(gases.rs) && gases.rs >= 0.0f && gases.rs <= 99999.999f) {
        mq["rs_kohm"] = numero(gases.rs, 0, 99999.999, 3);
      } else {
        mq["rs_kohm"] = nullptr;
      }
      mq["alarm"] = nullptr;  // el firmware no tiene alarma digital (D-out)

      struct { const char* gas; float ppm; } lista[] = {
        {"CO2", gases.co2}, {"CO", gases.co}, {"NH3", gases.nh3},
        {"ALCOHOL", gases.alcohol}, {"TOLUENE", gases.tolueno}, {"ACETONE", gases.acetona}
      };
      JsonArray estimaciones = mq.createNestedArray("estimates");
      for (const auto& g : lista) {
        JsonObject e = estimaciones.createNestedObject();
        e["gas"] = g.gas;
        if (ppmGuardable(g.ppm)) e["ppm"] = numero(g.ppm, 0, 99999999.99, 2);
        else                     e["ppm"] = nullptr;
      }
    }

    // MAX9814. Se envia desde la primera consolidacion (cada 3 s). Si el offset
    // esta fuera de rango llega valid=false y la API la guarda marcada como invalida.
    if (audio.timestamp != 0) {
      JsonObject a = doc.createNestedObject("audio");
      a["valid"]           = audio.valida;
      a["rms_mv"]          = numero(audio.rmsMv, 0, 99999.99, 2);
      a["peak_to_peak_mv"] = numero(audio.picoMv, 0, 99999.99, 2);
      a["offset_mv"]       = numero(audio.offsetMv, 0, 99999.9, 1);
      a["sample_rate_hz"]  = (uint32_t)(audio.frecuenciaHz > 65535 ? 65535 : audio.frecuenciaHz);
    }

    // Sin LDR: este firmware no lo tiene (GPIO34 no existe en el ESP32-S3).

    if (doc.overflowed()) Serial.println("[EnvioAPI] AVISO el JSON no cupo completo en el documento");

    String json;
    serializeJson(doc, json);
    return json;
  }

  // POST al endpoint de ingesta. Devuelve el codigo HTTP (negativo = error de conexion).
  int enviar(const String& cuerpo, String& respuesta) {
    WiFiClientSecure cliente;
    cliente.setCACert(CERTIFICADOS_RAIZ);
    cliente.setHandshakeTimeout(EnvioAPIConfig::TIMEOUT_MS / 1000);

    HTTPClient http;
    if (!http.begin(cliente, EnvioAPIConfig::URL)) {
      respuesta = "URL invalida";
      return -100;
    }
    http.setConnectTimeout(EnvioAPIConfig::TIMEOUT_MS);
    http.setTimeout(EnvioAPIConfig::TIMEOUT_MS);
    http.setUserAgent(String("BioRed-ESP32/") + EnvioAPIConfig::FIRMWARE_VERSION);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-api-key", EnvioAPIConfig::API_KEY);

    const int codigo = http.POST(cuerpo);
    if (codigo > 0) {
      respuesta = http.getString();
    } else {
      char tls[100] = "";
      cliente.lastError(tls, sizeof(tls));
      respuesta = http.errorToString(codigo);
      if (tls[0] != '\0') respuesta += String(" | TLS ") + tls;
    }
    http.end();
    return codigo;
  }

  const char* pista(int codigo) {
    switch (codigo) {
      case 400: return "El JSON no cumple el contrato. Revisa error.details en la respuesta";
      case 401: return "Key ausente o invalida. Revisa API_KEY en EnvioAPIConfig.h (debe ser la de dispositivo)";
      case 403: return "Cloudflare bloqueo la peticion (WAF o Bot Fight Mode). Falta la regla de excepcion para /api/v1/readings/ingest";
      case 404: return "Ruta no encontrada. Revisa URL en EnvioAPIConfig.h";
      case 422: return "Estacion no registrada o sin sensor activo. Revisa la MAC y los sensores en Postman";
      case 429: return "Demasiadas peticiones. Se reintenta con espera creciente";
      default:  return codigo >= 500 ? "Falla del servidor. Se reintenta con espera creciente"
                                     : "Sin conexion con el servidor (wifi, DNS, certificado o tiempo de espera)";
    }
  }

  // Segun el codigo decide cuanto esperar antes del siguiente envio (registro-estacion.md).
  uint32_t siguienteEspera(int codigo) {
    if (codigo == 201) {
      fallosSeguidos = 0;
      return EnvioAPIConfig::INTERVALO_MS;
    }
    // Errores de configuracion: reintentar no los arregla. Pausa larga hasta corregir.
    if (codigo == 400 || codigo == 401 || codigo == 403 || codigo == 404 || codigo == 422) {
      fallosSeguidos = 0;
      return EnvioAPIConfig::ESPERA_ERROR_MS;
    }
    // 429, 5xx y errores de conexion: 60 s, 120 s, 240 s... hasta el tope.
    if (fallosSeguidos < 5) fallosSeguidos++;
    const uint32_t espera = EnvioAPIConfig::INTERVALO_MS << fallosSeguidos;
    return espera > EnvioAPIConfig::ESPERA_MAXIMA_MS ? EnvioAPIConfig::ESPERA_MAXIMA_MS : espera;
  }

  void reportar(int codigo, const String& respuesta, uint32_t duracionMs, uint32_t espera) {
    if (codigo == 201) {
      estado.enviados++;
      StaticJsonDocument<384> r;
      long id = 0;
      if (deserializeJson(r, respuesta) == DeserializationError::Ok) id = r["data"]["reading_id"] | 0L;
      Serial.printf("[EnvioAPI] 201 lectura %ld guardada en %lu ms (total %lu)\n",
                    id, (unsigned long)duracionMs, (unsigned long)estado.enviados);
      return;
    }
    estado.fallidos++;
    Serial.printf("[EnvioAPI] Error %d en %lu ms. %s\n", codigo, (unsigned long)duracionMs, pista(codigo));
    Serial.printf("[EnvioAPI] Respuesta: %.200s\n", respuesta.c_str());
    Serial.printf("[EnvioAPI] Proximo intento en %lu s\n", (unsigned long)(espera / 1000));
  }

  void tareaEnvio(void*) {
    vTaskDelay(pdMS_TO_TICKS(ESPERA_INICIAL_MS));
    for (;;) {
      const uint32_t inicio = millis();
      uint32_t espera = EnvioAPIConfig::INTERVALO_MS;

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[EnvioAPI] Sin WiFi, se omite este envio");
      } else {
        const String cuerpo = armarJson();
        if (EnvioAPIConfig::MOSTRAR_JSON) Serial.printf("[EnvioAPI] JSON %s\n", cuerpo.c_str());

        String respuesta;
        const int codigo = enviar(cuerpo, respuesta);
        estado.ultimoCodigo  = codigo;
        estado.ultimoEnvioMs = inicio;
        espera = siguienteEspera(codigo);
        reportar(codigo, respuesta, millis() - inicio, espera);
      }

      // El intervalo se cuenta desde el inicio del envio, asi se mantiene en 30 s.
      const uint32_t transcurrido = millis() - inicio;
      vTaskDelay(pdMS_TO_TICKS(espera > transcurrido ? espera - transcurrido : 1000));
    }
  }
}

void envioApiInit() {
  if (tarea != nullptr) return;

  String mac = strlen(EnvioAPIConfig::DEVICE_UID) > 0 ? String(EnvioAPIConfig::DEVICE_UID) : WiFi.macAddress();
  mac.toUpperCase();
  strncpy(deviceUid, mac.c_str(), sizeof(deviceUid) - 1);

  if (strstr(EnvioAPIConfig::API_KEY, "PEGA_AQUI") != nullptr) {
    Serial.println("[EnvioAPI] Falta la key de dispositivo en EnvioAPIConfig.h. El envio queda desactivado.");
    return;
  }

  // Nucleo 0 (loop() corre en el 1). Prioridad baja: el WiFi y el resto van primero.
  const BaseType_t ok = xTaskCreatePinnedToCore(tareaEnvio, "EnvioAPI", STACK_TAREA, nullptr, 1, &tarea, 0);
  if (ok == pdPASS) {
    Serial.printf("[EnvioAPI] Enviando cada %lu s a %s como %s\n",
                  (unsigned long)(EnvioAPIConfig::INTERVALO_MS / 1000), EnvioAPIConfig::URL, deviceUid);
  } else {
    Serial.println("[EnvioAPI] No se pudo crear la tarea de envio (memoria insuficiente)");
    tarea = nullptr;
  }
}

const EstadoEnvioAPI& envioApiEstado() {
  return estado;
}
